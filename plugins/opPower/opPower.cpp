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
    info.name = "^";
    info.name_len = 1;
    info.aliases = aliases;
    info.alias_lens = alias_lens;
    info.alias_count = 0;
    info.min_args = 2;
    info.max_args = 2;
    info.func = [](unsigned argc, double const* argv, int* err, 
        char* err_msg, int err_msg_size) -> double
        {
        if(argc != 2){
            *err = 1;
            snprintf(err_msg, err_msg_size,
                "power operator expects 2 argument, get %d", argc);
            std::string host_err = "[ERROR] [plugins\\opPower.cpp::power] Expected 2 argument, got " + std::to_string(argc);
            host_->report_error(host_err.data(), host_err.size());

            return 0.0;
        }

        std::string host_log = "[INFO] [plugins\\opPower.cpp::ln] power operator sucesfully computed";
        host_->log(host_log.data(), host_log.size());

        return std::pow(argv[0], argv[1]);
    };
    info.abi_version = 1;
    info.description = "Power operator. Expect 2 argument. First - base, second - power";
    info.is_oper = true;
    info.is_right_assoc_oper = true;
    info.prec = 2;
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

    std::string host_log = "[INFO] [plugins\\opPower.cpp::plugin_init] Plugin opPower successfully initialized";
    host_->log(host_log.data(), host_log.size());

    return 0;
}

PLUGIN_EXPORT int plugin_shutdown(){
    if(!initialized){
        if(host_ && host_->log) {
            std::string host_err = "[ERROR] [plugins\\opPower.cpp::plugin_shutdown] opPower plugin_shutdown called before init";
            host_->report_error(host_err.data(), host_err.size());
        }
        return 1;
    }
    std::string host_log = "[INFO] [plugins\\opPower.cpp::plugin_shutdown] Plugin opPower successfuly shutdowned";
    host_->log(host_log.data(), host_log.size());
    initialized = false;
    host_ = nullptr;
    
    return 0;
}
