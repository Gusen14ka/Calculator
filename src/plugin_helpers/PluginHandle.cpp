#include "plugin_helpers/PluginHandle.hpp"
#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/platform.hpp"
#include <string>
#include <winerror.h>

double PluginHandle::call(unsigned argc, double const * argv, int* err, char* err_msg, int err_msg_size) const{
    if(!func){
        if(err){
            *err = 1;
            if(err_msg_size){
                snprintf(err_msg, err_msg_size, "no plugin function");
            }
        }
        return 0.0;
    }

    double res = func(argc, argv, err, err_msg, err_msg_size);
    if(err){
        if(*err != 0){
            //TODO: обработка возвращенной ошибки из плагина
            throw;
        }
    }
    return res;
}

int PluginHandle::do_init(HostApi const* host, std::string* out_err) noexcept{
    if(!init){
        initialized = true;
        //TODO: LOG.debug
        return 0;
    }
    char errbuf[ERROR_BUF_SIZE] = {0};
    int res = init(host, errbuf, ERROR_BUF_SIZE);
    if(res != 0){
        if(out_err){
            *out_err = std::string(errbuf[0] ? errbuf : "plugin_init in " + path + " failed");
            return 1.0;
        }
    }
    initialized = true;
    return 0.0;
}

int PluginHandle::do_shutdown(std::string* out_err) noexcept{
    if(shutdown && initialized){
        try{
            shutdown();
        }catch(...){
            if(out_err) *out_err = "plugin_shutdown in " + path + " failed";
            //TODO:
            // LOG.error
            initialized = false;
            return 1.0;
        }
    }
    initialized = false;
    return 0.0;
}

void PluginHandle::close_library() noexcept{
    if(lib){
        platform_free_library(lib);
        lib = nullptr;
    }
}