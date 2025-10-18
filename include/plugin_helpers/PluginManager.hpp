#pragma once

#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/PluginHandle.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>


class PluginManager{
public:
    PluginManager(HostApi const * host) : host_(host) {}
    ~PluginManager() noexcept;

    // Загрузка плагина по пути.
    bool load(std::filesystem::path const & path, std::string* out_err = nullptr);

    // Выгрузка плагина по пути
    bool unload_by_path(std::string const & path, std::string* out_err = nullptr);

    // Поиск по имени функции. Возвращаем shared_ptr чтобы обеспечить длительность жизни
    std::shared_ptr<PluginHandle> find(std::string const & name) const;

    // Проверяет папку с плагинами, если есть изменения - загружает
    void scan_directory(std::filesystem::path const & dir);

private:
    // Выгрузка плагина по имени функции
    bool unload_by_name(std::string const & path, std::string* out_err = nullptr);

    // Убираем пробелы с концов и приводим к нижнему регистру
    std::string normalize_name(std::string s) const;

    // Приводим строку к нижнему регистру
    std::string to_lower_str(std::string s) const;

    // Проверка расширения файла
    bool has_lib_extension(std::filesystem::path const & path) const;

    // Удаляем все бекапы
    void cleanup_temp_backups() noexcept;

    HostApi const * host_ = nullptr;
    
    // Основыным контейнером является by_path_, тк в by_name_ также зарегестрированы элиасы функций
    std::unordered_map<std::string, std::shared_ptr<PluginHandle>> by_name_;
    std::unordered_map<std::string, std::shared_ptr<PluginHandle>> by_path_;

    // Контейнер соответсвия между изначальным путём к плагину и к его временному бекапу
    std::unordered_map<std::string, std::filesystem::path> temp_backups_;
};