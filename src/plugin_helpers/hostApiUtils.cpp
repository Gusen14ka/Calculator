#include "plugin_helpers/hostApiUtils.hpp"
#include "logger/Logger.hpp"
#include <string>

#define LOG Logger::instance()

void host_log(const char *msg, unsigned msg_size){
    if(msg_size != 0){
        LOG.info(std::string(msg, msg_size), "");
    }
}

void host_report_error(const char *msg, unsigned int msg_size){
    if(msg_size != 0){
        LOG.error(std::string(msg, msg_size), "");
    }
}