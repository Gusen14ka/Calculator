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

PLUGIN_EXPORT int plugin_get_info(PluginInfo** const out_info){
    
    static char const* const aliases[] = {nullptr};
    static unsigned const alias_lens[]= {0};
    info.name = "ln";
    info.name_len = 2;
    info.aliases = aliases;
    info.alias_lens = alias_lens;
    info.alias_count = 0;
    info.min_args = 1;
    info.max_args = 1;
    info.func = [](unsigned argc, double const* argv, int* err, 
        char* err_msg, int err_msg_size) -> double
        {
        if(argc != 1){
            *err = 1;
            snprintf(err_msg, err_msg_size,
                "ln() expects 1 argument, get %d", argc);
            std::string host_err = "[ERROR] [plugins\\funcln.cpp::ln] Expected 1 argument, got " + std::to_string(argc);
            host_->report_error(host_err.data(), host_err.size());

            return 0.0;
        }
        if(argv[0] <= 0){
            *err = 1;
            snprintf(err_msg, err_msg_size,
                "ln() expects positive argument, get %lf", argv[0]);
            std::string host_err = "[ERROR] [plugins\\funcln.cpp::ln] Expected positive argument, got " + std::to_string(argv[0]);
            host_->report_error(host_err.data(), host_err.size());

            return 0.0;
        }
        std::string host_log = "[INFO] [plugins\\funcln.cpp::ln] ln function sucesfully computed";
        host_->log(host_log.data(), host_log.size());

        return std::log(argv[0]);
    };
    info.abi_version = 1;
    info.description = "Natural logarithm function. Expect 1 argument";
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

    std::string host_log = "[INFO] [plugins\\funcln.cpp::plugin_init] Plugin funcln successfully initialized";
    host_->log(host_log.data(), host_log.size());

    return 0;
}

PLUGIN_EXPORT int plugin_shutdown(){
    if(!initialized){
        if(host_ && host_->log) {
            std::string host_err = "[ERROR] [plugins\\funcln.cpp::plugin_shutdown] Funcln plugin_shutdown called before init";
            host_->report_error(host_err.data(), host_err.size());
        }
        return 1;
    }
    std::string host_log = "[INFO] [plugins\\funcln.cpp::plugin_shutdown] Plugin funcln successfuly shutdowned";
    host_->log(host_log.data(), host_log.size());
    initialized = false;
    host_ = nullptr;
    
    return 0;
}
