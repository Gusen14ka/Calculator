#include "plugin_helpers/PluginManager.hpp"
#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/PluginHandle.hpp"
#include "plugin_helpers/platform.hpp"
#include "logger/Logger.hpp"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>
#include <chrono>

#define LOG Logger::instance()

PluginManager::~PluginManager() noexcept{
    for(auto & el: by_path_){
        auto pl = el.second;
        if(pl){
            pl->do_shutdown();
            pl->close_library();
        }
    }
    by_name_.clear();
    by_path_.clear();
    cleanup_temp_backups();
}

bool PluginManager::load(std::filesystem::path const & path, std::string* out_err){
    // Загружаем библотеку
    LibHandle lib = platform_load_dll(path);
    
    if(!lib){
        if(out_err) *out_err = std::string("Failed to load library: ") + path.string();
        return false;
    }

    // Загружаем метаданные
    // Ищем метод выдающий методанные
    std::string err;
    auto sym = platform_get_symbol(lib, "plugin_get_info", &err);
    
    if(!sym){
        LOG.error("Error in platform_get_symbol - plugin_get_info is nullptr: " + path.string(), "PluginManager::load");
        platform_free_library(lib);
        if(out_err) *out_err = std::string("symbol plugin_get_info not found in: ") + path.string();
        return false;
    }

    // Берём методанные
    auto get_info = reinterpret_cast<plugin_get_info_t>(sym);
    PluginInfo * plug_info = nullptr;
    int res = get_info(&plug_info);

    if(res != 0){
        platform_free_library(lib);
        if (out_err) *out_err = std::string("plugin_get_info failed for: ") + path.string();
        LOG.error("Func plugin_get_info failed: " + path.string(), "PluginManager::load");
        return false;
    }

    // Проверяем их валидность
    if(!plug_info){
        platform_free_library(lib);
        if (out_err) *out_err = std::string("plugin_get_info returned null info for: ") + path.string();
        LOG.error("Plugin info is null ptr: " + path.string(), "PluginManager::load");
        return false;
    }

    if(plug_info->func == nullptr) {
        platform_free_library(lib);
        if (out_err) *out_err = std::string("Plugin has no function pointer for: ") + path.string();
        LOG.error("Plugin function is null ptr: " + path.string(), "PluginManager::load");
        return false;
    }
    
    if(plug_info->abi_version != ABI_VERSION){
        platform_free_library(lib);
        if (out_err) *out_err = std::string("ABI version mismatch for: ") + path.string();
        LOG.error("ABI version mismatch: " + path.string(), "PluginManager::load");
        return false;
    }

    std::vector<std::string> names;
    if(plug_info->name){
        if(plug_info->name_len == 0){
            if(out_err) *out_err = "Function has null length name: " + path.string();
            LOG.error("Function has null length name: " + path.string(), "PluginManager::load");
            return false;
        }
        names.emplace_back(std::string(plug_info->name, plug_info->name_len));
    }
    else{
        if(out_err) *out_err = "Function has null name: " + path.string();
        LOG.error("Function has null name: " + path.string(), "PluginManager::load");
        return false;
    }


    if(plug_info->alias_count != 0){
        if(!plug_info->aliases){
            if(out_err) *out_err = "Function has null aliases ptr" + path.string();
            LOG.error("Function has null aliases ptr but not null alias_count: " + path.string(), "PluginManager::load");
            return false;
        }
        for(std::size_t i = 0; i < plug_info->alias_count; ++i){
            if(!plug_info->aliases[i] || !plug_info->alias_lens[i]){
                if(out_err) *out_err = "Dismatch in aliases and its count" + path.string();
                LOG.error("Dismatch in aliases and its count: " + path.string(), "PluginManager::load");
                return false;
            }
            if(plug_info->alias_lens[i] == 0){
                if(out_err) *out_err = "Dismatch in aliases and its lens" + path.string();
                LOG.error("Dismatch in aliases and its lens: " + path.string(), "PluginManager::load");
                return false;
            }
            names.emplace_back(std::string(plug_info->aliases[i], plug_info->alias_lens[i]));
        }
    }

    // Готовим PluginHandle
    auto plug_handle = std::make_shared<PluginHandle>();
    plug_handle->lib = lib;
    plug_handle->path = path.string();
    plug_handle->names = std::move(names);
    plug_handle->info = plug_info;
    plug_handle->func = plug_info->func;
    // TODO:
    // Будем ли как-то отсылать вверх err из platform_get_symbol + LOG или оставить лог только в самой функции
    plug_handle->init = reinterpret_cast<plugin_init_t>(platform_get_symbol(lib, "plugin_init"));
    if(!plug_handle->init){
        LOG.warning("platform_get_symbol - plugin_init is nullptr: " + path.string(), "PluginManager::load");
    }
    plug_handle->shutdown = reinterpret_cast<plugin_shutdown_t>(platform_get_symbol(lib, "plugin_shutdown"));
    if(!plug_handle->shutdown){
        LOG.warning("platform_get_symbol - plugin_shutdown is nullptr: " + path.string(), "PluginManager::load");
    }
    std::error_code ec;
    plug_handle->last_write = std::filesystem::last_write_time(path, ec);
    if(ec){
        LOG.error("Error code in std::filesystem::last_write_time: " + std::to_string(ec.value()) + ": " + path.string() , "PluginManager::load");
        plug_handle->last_write = {};
    }


    // Добавляем в мапу с проверкой на конфикт имён
    for(auto & el: plug_handle->names){
        auto norm_name = normalize_name(el);
        if(by_name_.count(norm_name)){
            plug_handle->close_library();
            if (out_err) *out_err = std::string("name conflict: ") + norm_name;
            LOG.error("Name conflict: " + norm_name + ": " + path.string(), "PluginManager::load");
            return false;
        }
        by_name_[norm_name] = plug_handle;
    }
    by_path_[path.string()] = plug_handle;

    // Вызовем инициализатор плагина, если такой есть
    res = plug_handle->do_init(host_, out_err);
    if(res != 0){
        LOG.error("Plugin init failed: " + path.string(), "PluginManager::load");
        if(!unload_by_path(path.string(), &err)){
            LOG.error("Unloading plugin by path failed: " + path.string(), "PluginManager::load");
        }
        return false;
    }
    
    LOG.info("Plugin successfully loaded: " + path.string(), "PluginManager::load");
    return true;
}

std::string PluginManager::normalize_name(std::string s) const{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){
        return !std::isspace(c);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){
        return !std::isspace(c);
    }).base(), s.end());

    std::transform(s.begin(), s.end(),s.begin(), [](unsigned char c){
        return std::tolower(c);
    });

    return s;
}
bool PluginManager::unload_by_path(std::string const & path, std::string* out_err){
    auto it = by_path_.find(path);
    if(it == by_path_.end()){
        if(out_err) *out_err = "not found in map: " + path;
        LOG.warning("Plugin not found in map: " + path, "PluginManager::unload_by_path");
        return false;
    }
    bool res_by_name = true;
    auto plugin_handle = it->second;
    for(auto& el: plugin_handle->names){
        res_by_name = unload_by_name(el, out_err) ? res_by_name : false;
    }
    // TODO:
    // Решить что делать в случае ошибке удаления по псевдониму: удалять по пути или нет?
    // Пока что принято решение - в любом случае удалять по пути
    by_path_.erase(it);

    plugin_handle->do_shutdown();
    plugin_handle->close_library();
    LOG.info("Plugin successfully deleted from maps, shutdowned and its library closed", "PluginManager::unload_by_path");
    return true;
}

bool PluginManager::unload_by_name(std::string const & name, std::string * out_err){
    std::string norm_name = normalize_name(name);
    auto it = by_name_.find(norm_name);
    if(it == by_name_.end()){
        if(out_err) *out_err = "not found by name: " + name;
        LOG.warning("Plugin name not found in map: " + name, "PluginManager::unload_by_name");
        return false;
    }
    by_name_.erase(it);
    LOG.info("Plugin name successfully deleted: " + name, "PluginManager::unload_by_name");
    return true;
}

std::shared_ptr<PluginHandle> PluginManager::find(std::string const& name) const{
    auto it = by_name_.find(normalize_name(name));
    if(it == by_name_.end()){
        return nullptr;
    }
    return it->second;
}

std::string PluginManager::to_lower_str(std::string s) const{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return s;
}

bool PluginManager::has_lib_extension(std::filesystem::path const & path) const{
    auto fname = path.filename().string();
    if(fname.empty()) return false;
    if (fname[0] == '.') return false; // .hidden
    if (fname.find('~') != fname.npos) return false; // backup/temp with ~
    if (fname.find(".tmp") != fname.npos) return false; // .tmp
    if (fname.find(".part") != fname.npos) return false;

    #ifdef _WIN32
        auto ext = to_lower_str(path.extension().string());
        return ext == ".dll";
    #else
        auto ext = to_lower_str(path.extension().string());
        return ext == ".so";
    #endif
}

void PluginManager::scan_directory(std::filesystem::path const & dir){
    std::error_code ec;
    for(auto& entry: std::filesystem::directory_iterator(dir, ec)){
        if(ec){
            LOG.error("Error code in std::filesystem::directory_iterator: " + std::to_string(ec.value()) + ": " + dir.string(), "PluginManager::scan_directory");
            break;
        }
        if(!entry.is_regular_file(ec) || ec){
            if(ec){
                LOG.warning("Error code in std::filesystem::is_regular_file: " + std::to_string(ec.value()) + ": " + entry.path().string(), "PluginManager::scan_directory");
            }
            continue;
        }

        auto path = entry.path();

        if(!has_lib_extension(path)){
            continue;
        }

        auto new_time = std::filesystem::last_write_time(path, ec);
        if(ec){
            LOG.error("Error code in std::filesystem::last_write_time: " + std::to_string(ec.value()) + ": " + path.string(), "PluginManager::scan_directory");
            continue;
        }

        // Проверяем добавлен ли уже этот плагин, если нет - добавляем
        // Если да, перегружаем на новую версию 
        auto it = by_path_.find(path.string());
        if(it == by_path_.end()){
            LOG.info("Found new plugin: " + path.string(), "PluginManager::scan_directory");

            std::string err;
            auto res = load(path, &err);
            if(!res){
                LOG.warning("Plugin failed loading: " + path.string(), "PluginManager::scan_directory");
                continue;
            }
        }
        
        LOG.info("Found loaded plugin: " + path.string(), "PluginManager::scan_directory");
        it = by_path_.find(path.string());
        // Проверяем на изменение файла
        auto plugin_handle = it->second;
        auto diff = new_time > plugin_handle->last_write ?
            new_time - plugin_handle->last_write : plugin_handle->last_write - new_time;
        if(diff <= std::chrono::seconds(1)){
            LOG.info("Found file is unchanged: " + path.string(), "PluginManager::scan_directory");
            continue;
        }

        // Файл изменён - перезаписываем
        /*
        Скорее всего чтобы реально безопасно перезаписывать нужно
        1. Выгрузить во временный файл уже загруженный dll
        2. unload_by_path() - удаляем изначальный длл
        3. load() - загружаем новый
        4. Если неудачно - выгружаем новый
        5. Загружаем изначальный из временного файла
        4.2 Если удачно - удаляем временный файл
        6. Поддержать удаление всех временных файлов при завершении приложения
        */
        LOG.info("Found file has changed: " + path.string(), "PluginManager::scan_directory");
        std::filesystem::path tmp_path = std::filesystem::temp_directory_path() / (path.filename().string() + ".backup.");
        
        // Создадим уникальное имя
        unsigned idx = 0;
        std::filesystem::path tmp_path_candidate;
        do{
            tmp_path_candidate = tmp_path.string() + std::to_string(idx++) + ".tmp";
        }while(std::filesystem::exists(tmp_path_candidate));
        tmp_path = tmp_path_candidate;

        // Бекапаем оригинал
        if(!std::filesystem::copy_file(path, tmp_path, std::filesystem::copy_options::none, ec) || ec){
            std::string err = "Error in std::filesystem::copy_file: " + path.string();
            if(ec) err += " Error code: " + std::to_string(ec.value());
            LOG.error(err, "PluginManager::scan_directory");
            continue;
        }
        temp_backups_[path.string()] = tmp_path;

        // Выгружаем оригнал 
        std::string err;
        if(!unload_by_path(path.string(), &err)){
            LOG.warning("Previous version plugin failed unloading: " + path.string(), "PluginManager::scan_directory");
            continue;
        }
        
        // Загружаем новую версию
        err = "";
        if(load(path, &err)){
            // Успех => удаляем бекап
            std::filesystem::remove(tmp_path, ec);
            temp_backups_.erase(path.string());
            LOG.info("New version of plugin successfully loaded: " + path.string(), "PluginManager::scan_directory");
            continue;
        }
        

        LOG.warning("New version of plugin failed loading: " + path.string(), "PluginManager::scan_directory");
        // Неуспех => перезаписываем - вместо новой бекап
        std::filesystem::copy_file(tmp_path, path, std::filesystem::copy_options::overwrite_existing, ec);
        if(!ec){
            if(load(path, &err)){
                std::filesystem::remove(tmp_path, ec);
                if(ec){
                    LOG.warning("Temporary plugin file failed deleting: " + tmp_path.string(), "PluginManager::scan_directory");
                    continue;
                }
                temp_backups_.erase(path.string());
                LOG.info("Previous version of plugin successfully loaded: " + path.string(), "PluginManager::scan_directory");
                continue;
            }
            else{
                LOG.error("Previous version of plugin failed loading: " + path.string(), "PluginManager::scan_directory");
                continue;
            }
        }
        else{
            LOG.warning("Failed coping from temporery file: " + tmp_path.string(), "PluginManager::scan_directory");
            // Если будут проблемы можно попробовать загружать плагин из временного файла без копирования
            if(load(tmp_path, &err)){
                std::filesystem::remove(path, ec);
                if(ec){
                    LOG.warning("New plugin file failed deleting: " + path.string(), "PluginManager::scan_directory");
                    continue;
                }
                temp_backups_.erase(tmp_path.string());
                LOG.info("Previous version of plugin successfully loaded from temporary file. New version deleted: " + tmp_path.string(), "PluginManager::scan_directory");
                continue;
            }
            else{
                LOG.error("Previous version of plugin failed loading loaded from temporary file: " + tmp_path.string(), "PluginManager::scan_directory");
                continue;
            }
        }
    }
}

void PluginManager::cleanup_temp_backups() noexcept{
    for(auto& tmp : temp_backups_){
        std::error_code ec;
        std::filesystem::remove(tmp.second, ec);
        if(ec){
            LOG.warning("Error code in std::filesystem::remove backup: " + tmp.second.string(), "PluginManager::cleanup_temp_backups");
        }
    }
    temp_backups_.clear();
}
