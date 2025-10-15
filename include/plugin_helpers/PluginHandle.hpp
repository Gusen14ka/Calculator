#pragma once

#include "PluginAPI.h"
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
    using LibHandle = HMODULE;
#else
    #include <dlfcn.h>
    using LibHandle = void*;
#endif

struct PluginHandle{
    LibHandle lib = nullptr;
    std::string path;
    std::vector<std::string> names;
    const PluginInfo* info = nullptr; // Не владеющий указатель на метаданные плагина внутри dll
    plugin_init_t init = nullptr;
    plugin_shutdown_t shutdown = nullptr;
    plugin_func_t func = nullptr;
    std::filesystem::file_time_type last_write{}; // Временная точка для того, чтобы не перезагружать неизменный плагин каждую итерацию
    bool initialized = false;

    // Вызов функции из плагина
    double call(unsigned argc, double const * argv, int* err, char* err_msg, int err_msg_size) const;

    int do_init(HostApi const* host, std::string* out_err = nullptr) noexcept;

    int do_shutdown(std::string* out_err = nullptr) noexcept;

    void close_library() noexcept;
};