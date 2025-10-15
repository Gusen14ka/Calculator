#pragma once

#include "PluginHandle.hpp"
#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/PluginHandle.hpp"
#include <filesystem>
#include <memory>
#include <unordered_map>

using PluginPtr = std::shared_ptr<PluginHandle>;

class PluginManager{
public:
    PluginManager(HostApi const * host) : host_(host) {}
    ~PluginManager() noexcept;

    // Загрузка плагина по пути.
    bool load(std::filesystem::path const & path, std::string* out_err = nullptr);

    // Выгрузка плагина по пути
    bool unload_by_path(std::filesystem::path const & path, std::string* out_err = nullptr);

    // Выгрузка плагина по имени функции
    bool unload_by_name(std::string const & path, std::string* out_err = nullptr);

    // Поиск по имени функции. Возвращаем shared_ptr чтобы обеспечить длительность жизни
    std::shared_ptr<PluginHandle> find(std::string const & name);

    // Проверяет папку с плагинами, если есть изменения - загружает
    void scan_directory(std::filesystem::path const & dir);

private:
    bool verify_and_init_plugin(std::shared_ptr<PluginHandle> p, std::string* out_err);
    void finish_unload(std::shared_ptr<PluginHandle> p); // shutdown + dlclose

    // Убираем пробелы с концов и приводим к нижнему регистру
    std::string normalize_name(std::string s);

    HostApi const * host_ = nullptr;
    
    // Основыным контейнером является by_path_, тк в by_name_ также зарегестрированы элиасы функций
    std::unordered_map<std::string, std::shared_ptr<PluginHandle>> by_name_;
    std::unordered_map<std::string, std::shared_ptr<PluginHandle>> by_path_;
};