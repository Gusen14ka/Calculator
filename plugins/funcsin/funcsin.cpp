#include "plugin_helpers/PluginAPI.h"
#include <cstdio>
#include <cmath>
#include <string>

#ifndef BUILD_PLUGIN
#define BUILD_PLUGIN
#endif

static PluginInfo info;
static HostApi* host_;
static bool initialized = false;

static constexpr double PI = 3.14159265358979323846;

PLUGIN_EXPORT int plugin_get_info(PluginInfo** const out_info){
    
    if(!initialized){
        if(out_info) *out_info = nullptr;
        if(host_ && host_->report_error){
            std::string err = "[ERROR] [plugins\\funcsin.cpp::plugin_get_info] plugin_get_info called before init";
            host_->report_error(err.data(), err.size());
        }
        return 1;
    }
    
    static char const* const aliases[] = {"sinus", nullptr};
    static unsigned const alias_lens[]= {5};
    info.name = "sin";
    info.name_len = 3;
    info.aliases = aliases;
    info.alias_lens = alias_lens;
    info.alias_count = 1;
    info.min_args = 1;
    info.max_args = 1;
    info.func = [](unsigned argc, double const* argv, int* err, 
        char* err_msg, int err_msg_size) -> double
        {
        if(argc != 1){
            *err = 1;
            snprintf(err_msg, err_msg_size,
                "sin() expects 1 argument, get %d", argc);
            std::string host_err = "[ERROR] [plugins\\funcsin.cpp::sin] Expected 1 argument, got " + std::to_string(argc);
            host_->report_error(host_err.data(), host_err.size());

            return 0.0;
        }
        std::string host_log = "[INFO] [plugins\\funcsin.cpp::sin] sin function sucesfully computed";
        host_->log(host_log.data(), host_log.size());

        return std::sin((argv[0] / 180) * PI);
    };
    info.abi_version = 1;
    info.description = "Sine function. Expect 1 argument in degrees";
    *out_info = &info;
    return 0;
}

PLUGIN_EXPORT int plugin_init(HostApi const * host, char* err_msg,
    int err_msg_size)
{
    
    if(!host || !host->log || !host->report_error){
        if(err_msg && err_msg_size > 0){
            snprintf(err_msg, err_msg_size, "Invalid HostApi pointer");
        }
        return 1; // ошибка инициализации
    }

    host_ = const_cast<HostApi*>(host);
    
    initialized = true;

    std::string host_log = "[INFO] [plugins\\funcsin.cpp::plugin_init] Plugin funcsin successfully initialized";
    host_->log(host_log.data(), host_log.size());

    return 0;
}

PLUGIN_EXPORT int plugin_shutdown(){
    if(!initialized){
        if(host_ && host_->log) {
            std::string host_err = "[ERROR] [plugins\\funcsin.cpp::plugin_shutdown] Funcsin plugin_shutdown called before init";
            host_->report_error(host_err.data(), host_err.size());
        }
        return 1;
    }
    std::string host_log = "[INFO] [plugins\\funcsin.cpp::plugin_shutdown] Plugin funcsin successfuly shutdowned";
    host_->log(host_log.data(), host_log.size());
    initialized = false;
    host_ = nullptr;
    
    return 0;
}
