#include "plugin_helpers/PluginHandle.hpp"
#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/platform.hpp"
#include <string>
#include <winerror.h>

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

std::string PluginHandle::name(std::string* err_out) const {
    if(info){
        return std::string(info->name, info->name_len);
    }
    //TODO LOG.error
    if(err_out) *err_out = "Pugin_info is nullptr: " + path;
    return "";
}

std::pair<int, int> PluginHandle::arity(std::string* err_out) const{
    if(info){
        return {info->min_args, info->max_args};
    }
    //TODO LOG.error
    if(err_out) *err_out = "Pugin_info is nullptr: " + path;
    return {};
}

Precedence PluginHandle::precedence(std::string* err_out) const{
    if(info){
        return Precedence::ZERO;
    }
    //TODO LOG.error
    if(err_out) *err_out = "Pugin_info is nullptr: " + path;
    return {};
}

bool PluginHandle::is_right_assoc_operator(std::string* err_out) const{
    if(info){
        return false;
    }
    //TODO LOG.error
    if(err_out) *err_out = "Pugin_info is nullptr: " + path;
    return {};
}

double PluginHandle::call(std::vector<double> const & args, std::string* err_out){
    if(!info){
        if(err_out) *err_out = "Pugin_info is nullptr: " + path;
        return {};
    }
    // Подготавливаем аргументы
    int err_code = 0;
    char err_msg[ERROR_BUF_SIZE] = {0};
    double res = 0.0;

    if(!func){
        if(err_out) *err_out = "no plugin function" + path;
        //TODO: LOG.error
        return 0.0;
    }

    unsigned argc = args.size();
    if(!((info->max_args == -1 && argc >= info->min_args) || 
        (argc >= info->min_args && argc <= info->max_args)))
    {
        if(err_out) *err_out = "Arguments mismatch" + path;
        //TODO: LOG.error
        return 0.0;
    }

    // Вообще не планируется что плагины будут кидать исключения, но на всякий поддерживаем
    try{
        res = func(argc, args.data(), &err_code, err_msg, ERROR_BUF_SIZE);
    }
    catch (const std::exception& e) {
        //TODO: LOG.error
        if(err_out) *err_out = "plugin threw exception: " + std::string(e.what());
        if(err_out && err_code != 0) *err_out += " Error code: " + std::to_string(err_code);
        if(err_out && err_msg[0] != '0') *err_out += " Error message: " + std::string(err_msg);
        return 0.0;
    } catch (...) {
        //TODO: LOG.error
        if(err_out) *err_out = "plugin threw unknown exception from function: " + path;
        if(err_out && err_code != 0) *err_out += " Error code: " + std::to_string(err_code);
        if(err_out && err_msg[0] != '0') *err_out += " Error message: " + std::string(err_msg);
        return 0.0;
    }

    if(err_code != 0){
        //TODO: LOG.error
        if(err_out && err_code != 0) *err_out =  "Plugin returned error Error code: " + std::to_string(err_code);
        if(err_out && err_msg[0] != '0') *err_out += " Error message: " + std::string(err_msg);
        return 0.0;
    }

    return res;
}



