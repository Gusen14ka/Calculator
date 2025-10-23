#include "plugin_helpers/PluginHandle.hpp"
#include "ICallable.hpp"
#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/platform.hpp"
#include "logger/Logger.hpp"
#include <string>
#include <winerror.h>
#include <algorithm>

#define LOG Logger::instance()

int PluginHandle::do_init(HostApi const* host, std::string* out_err) noexcept{
    LOG.info("Plugin init start", "PluginHandle::do_init");
    if(!init){
        initialized = true;
        LOG.warning("Plugin hasn't init_func: " + path, "PluginHandle::do_init");
        return 0;
    }
    char errbuf[ERROR_BUF_SIZE] = {0};
    int res = init(host, errbuf, ERROR_BUF_SIZE);
    if(res != 0){
        if(out_err){
            *out_err = std::string(errbuf[0] ? errbuf : "plugin_init in " + path + " failed");
            LOG.error("Plugin init failed: " + path, "PluginHandle::do_init");
            return 1.0;
        }
    }
    initialized = true;
    LOG.info("Plugin succesfully inited: " + path,"PluginHandle::do_init");
    return 0.0;
}

int PluginHandle::do_shutdown(std::string* out_err) noexcept{
    LOG.info("Plugin shutdown start", "PluginHandle::do_shutdown");
    if(shutdown && initialized){
        try{
            shutdown();
        }catch(...){
            if(out_err) *out_err = "plugin_shutdown in " + path + " failed";
            LOG.error("Plugin failed shutdown: " + path, "PluginHandle::do_shutdown");
            initialized = false;
            return 1.0;
        }
    }
    initialized = false;
    LOG.info("Plugin successfully shutdowned: " + path, "PluginHandle::do_shutdown");
    return 0.0;
}

void PluginHandle::close_library() noexcept{
    if(lib){
        platform_free_library(lib);
        lib = nullptr;
    }
    LOG.info("Successfulle closed library: " + path, "PluginHandle::close_library");
}

std::string PluginHandle::name(std::string* err_out) const {
    if(info){
        LOG.info("Successfully get plugin name: " + path, "PluginHandle::name");
        return std::string(info->name, info->name_len);
    }
    if(err_out) *err_out = "Plugin_info is nullptr: " + path;
    LOG.error("Plugin info is nullptr: " + path, "PluginHandle::name");
    return "";
}

std::pair<int, int> PluginHandle::arity(std::string* err_out) const{
    if(info){
        LOG.info("Successfully get plugin arity: " + path, "PluginHandle::arity");
        return {info->min_args, info->max_args};
    }
    if(err_out) *err_out = "Pugin_info is nullptr: " + path;
    LOG.error("Plugin info is nullptr: " + path, "PluginHandle::arity");
    return {};
}

Precedence PluginHandle::precedence(std::string* err_out) const{
    if(info){
        LOG.info("Successfully get plugin precedence: " + path, "PluginHandle::precedence");
        if(info->is_oper){
            return Precedence((std::min)(info->prec, unsigned(3)));
        }
        else{
            return Precedence::THIRD;
        }
    }
    if(err_out) *err_out = "Pugin_info is nullptr: " + path;
    LOG.error("Plugin info is nullptr: " + path, "PluginHandle::precedence");
    return {};
}

bool PluginHandle::is_right_assoc_operator(std::string* err_out) const{
    if(info){
        LOG.info("Successfully get plugin precedence: " + path, "PluginHandle::is_right_assoc_operator");
        return info->is_right_assoc_oper;
    }
    if(err_out) *err_out = "Pugin_info is nullptr: " + path;
    LOG.error("Plugin info is nullptr: " + path, "PluginHandle::is_right_assoc_operator");
    return {};
}

double PluginHandle::call(std::vector<double> const & args, std::string* err_out){
    if(!info){
        if(err_out) *err_out = "Pugin_info is nullptr: " + path;
        LOG.error("Plugin info is nullptr: " + path, "PluginHandle::call");
        return {};
    }
    // Подготавливаем аргументы
    int err_code = 0;
    char err_msg[ERROR_BUF_SIZE] = {0};
    double res = 0.0;

    if(!func){
        if(err_out) *err_out = "no plugin function" + path;
        LOG.error("Plugin function is nullptr: " + path, "PluginHandle::call");
        return 0.0;
    }

    unsigned argc = args.size();
    if(!((info->max_args == -1 && argc >= info->min_args) || 
        (argc >= info->min_args && argc <= info->max_args)))
    {
        if(err_out) *err_out = "Arguments mismatch" + path;
        LOG.error("Plugin function arguments mismatch: " + path, "PluginHandle::call");
        return 0.0;
    }

    // Вообще не планируется что плагины будут кидать исключения, но на всякий поддерживаем
    try{
        res = func(argc, args.data(), &err_code, err_msg, ERROR_BUF_SIZE);
    }
    catch (const std::exception& e) {
        std::string err = "Plugin" + path + " threw exception: " + std::string(e.what());
        if(err_code != 0) err += " Error code: " + std::to_string(err_code);
        if(err_msg[0] != '0') err += " Error message: " + std::string(err_msg);
        if(err_out) *err_out =  err;
        LOG.error(err, "PluginHandle::call");
        return 0.0;
    } catch (...) {

        std::string err = "Plugin threw unknown exception from function: " + path;
        if(err_code != 0) err += " Error code: " + std::to_string(err_code);
        if(err_msg[0] != '0') err += " Error message: " + std::string(err_msg);
        if(err_out) *err_out = err;
        LOG.error(err, "PluginHandle::call");
        return 0.0;
    }

    if(err_code != 0){
        std::string err, user_err;
        if(err_code != 0) err =  "Plugin returned error Error code: " + std::to_string(err_code);
        if(err_msg[0] != '0'){
            err += " Error message: " + std::string(err_msg);
            user_err = std::string(err_msg);
        } 
        if(err_out) *err_out = user_err;
        LOG.error(err, "PluginHandle::call");
        return 0.0;
    }

    return res;
}



