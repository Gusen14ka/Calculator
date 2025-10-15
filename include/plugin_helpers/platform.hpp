#pragma once

#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
    using LibHandle = HMODULE;
#else
    #include <dlfcn.h>
    using LibHandle = void*;
#endif

// Загрузка библиотеки
LibHandle platform_load_dll(std::filesystem::path const & path) noexcept;

// Возвращает возможную ошибку во время выполнения системных функций по работе с dll
std::string last_dll_error();

// Закрытие библеотеки
void platform_free_library(LibHandle handle) noexcept;

// Возвращает символ - указатель на искомую функцию из плагина
void* platform_get_symbol(LibHandle handle, char const * name, std::string* out_err = nullptr) noexcept;