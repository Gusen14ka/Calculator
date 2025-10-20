#pragma once

#include "PluginAPI.h"
#include "ICallable.hpp"
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

class PluginHandle : public ICallable{
public:
    LibHandle lib = nullptr;
    std::string path;
    std::vector<std::string> names;
    const PluginInfo* info = nullptr; // Не владеющий указатель на метаданные плагина внутри dll
    plugin_init_t init = nullptr;
    plugin_shutdown_t shutdown = nullptr;
    plugin_func_t func = nullptr;
    std::filesystem::file_time_type last_write{}; // Временная точка для того, чтобы не перезагружать неизменный плагин каждую итерацию
    bool initialized = false;

    int do_init(HostApi const* host, std::string* out_err = nullptr) noexcept;

    int do_shutdown(std::string* out_err = nullptr) noexcept;

    void close_library() noexcept;

    // Оверрайд функций из интерфейса
    std::string name(std::string* err_out) const override;
    std::pair<int, int> arity(std::string* err_out) const override;
    Precedence precedence(std::string* err_out) const override;
    bool is_right_assoc_operator(std::string* err_out) const override;
    double call(std::vector<double> const & args, std::string* err_out) override;
};