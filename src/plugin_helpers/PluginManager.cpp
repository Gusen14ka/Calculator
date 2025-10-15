#include "plugin_helpers/PluginManager.hpp"
#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/PluginHandle.hpp"
#include "plugin_helpers/platform.hpp"
#include <exception>
#include <filesystem>
#include <memory>
#include <vector>


PluginManager::~PluginManager() noexcept{
    for(auto & el: by_path_){
        auto pl = el.second;
        if(pl){
            //TODO: обработка ошибок, если нужна на этом уровне
            pl->do_shutdown();
            pl->close_library();
        }
    }
    by_name_.clear();
    by_path_.clear();
}

bool PluginManager::load(std::filesystem::path const & path, std::string* out_err){
    // Загружаем библотеку
    LibHandle lib = platform_load_dll(path);
    
    if(!lib){
        if(out_err) *out_err = std::string("failed to load library: ") + path.string();
        //TODO:
        //LOG.error
        return false;
    }

    // Загружаем метаданные
    // Ищем метод выдающий методанные
    std::string err;
    auto sym = platform_get_symbol(lib, "plugin_get_info", &err);
    
    if(!sym){
        // TODO:
        // Будем ли как-то отсылать вверх err из platform_get_symbol + LOG
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
        //TODO:
        // LOG
        return false;
    }

    // Проверяем их валидность
    if(!plug_info){
        platform_free_library(lib);
        if (out_err) *out_err = std::string("plugin_get_info returned null info for: ") + path.string();
        //TODO:
        // LOG
        return false;
    }

    if(plug_info->func == nullptr) {
        platform_free_library(lib);
        if (out_err) *out_err = std::string("plugin has no function pointer for: ") + path.string();
        //TODO:
        // LOG
        return false;
    }
    
    if(plug_info->abi_version != ABI_VERSION){
        platform_free_library(lib);
        if (out_err) *out_err = std::string("ABI version mismatch for: ") + path.string();
        //TODO:
        // LOG
        return false;
    }

    std::vector<std::string> names;
    if(plug_info->name){
        if(plug_info->name_len == 0){
            if(out_err) *out_err = "function has null length: " + path.string();
            //TODO:
            // LOG
            return false;
        }
        names.emplace_back(std::string(plug_info->name, plug_info->name_len));
    }
    else{
        if(out_err) *out_err = "function has null name: " + path.string();
        //TODO:
        // LOG
        return false;
    }


    if(plug_info->alias_count != 0){
        if(!plug_info->aliases){
            if(out_err) *out_err = "function has null aliases ptr" + path.string();
            //TODO:
            // LOG
            return false;
        }
        for(std::size_t i = 0; i < plug_info->alias_count; ++i){
            if(!plug_info->aliases[i] || !plug_info->alias_lens[i]){
                if(out_err) *out_err = "dismatch in aliases and its count" + path.string();
                //TODO:
                // LOG
                return false;
            }
            if(plug_info->alias_lens[i] == 0){
                if(out_err) *out_err = "dismatch in aliases and its lens" + path.string();
                //TODO:
                // LOG
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
    plug_handle->shutdown = reinterpret_cast<plugin_shutdown_t>(platform_get_symbol(lib, "plugin_shutdown"));
    try{
        plug_handle->last_write = std::filesystem::last_write_time(path);
    }catch(std::exception & e){
        //TODO:
        //LOG.error
    }

    // Добавляем в мапу с проверкой на конфикт имён
    for(auto & el: plug_handle->names){
        auto norm_name = normalize_name(el);
        if(by_name_.count(norm_name)){
            plug_handle->close_library();
            if (out_err) *out_err = std::string("name conflict: ") + norm_name;
            //TODO:
            //LOG.error
            return false;
        }
        by_name_[norm_name] = plug_handle;
        by_path_[path.string()] = plug_handle;
    }

    // Вызовем инициализатор плагина, если такой есть
    if(plug_handle->init){
        int res = plug_handle->do_init(host_, out_err);
        if(res != 0){
            by_path_.erase(path.string());
            plug_handle->close_library();
            //TODO:
            //LOG.error
            return false;
        }
    }
    //TODO: LOG.info
    return true;
}

std::string PluginManager::normalize_name(std::string s){
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