#include "functional/Executor.hpp"
#include "functional/Parser.hpp"
#include "include/operators/OperatorRegistry.hpp"
#include "plugin_helpers/PluginAPI.h"
#include "plugin_helpers/PluginManager.hpp"
#include "functional/Lexer.hpp"
#include "libs/tinyfiledialogs/tinyfiledialogs.h"
#include "logger/Logger.hpp"
#include "plugin_helpers/hostApiUtils.hpp"
#include <filesystem>
#include <string>
#include <iostream>

#define LOG Logger::instance()

void critical_error_call(std::string const & err){
    tinyfd_messageBox("Error", err.c_str(), "ok", "error", 1);
}

int main(){
    // Настраиваем логгер
    LOG.set_log_to_console(false);
    // Папка build
    std::string projectRoot = std::filesystem::current_path().parent_path().string();
    
    LOG.set_log_file(projectRoot + "\\log.txt");
    LOG.set_log_to_file(true);

    LOG.info("Application started", "Main");

    std::filesystem::path plugin_dir = projectRoot + "\\plugins";

    // Сформируем хост апи
    HostApi host = {
        host_log,
        host_report_error
    };

    PluginManager plugin_manage(&host);
    plugin_manage.scan_directory(plugin_dir);

    OperatorRegistry op_reg;
    op_reg.register_builtin_operators();

    while(true){
        plugin_manage.scan_directory(plugin_dir);
        std::string input, err;
        std::cout << "Enter math expression:\n";
        std::getline(std::cin, input);
        if(input.empty()) break;

        Lexer lex(std::move(input));
        Parser parser(op_reg, plugin_manage);
        Executor executor;

        auto tokens = lex.tokenize(err);
        if(!err.empty()){
            critical_error_call(err);
            continue;
        }
        auto items = parser.shunting_yard(tokens, err);
        if(!err.empty()){
            critical_error_call(err);
            continue;
        }
        auto res = executor.execute(items, err);
        if(!err.empty()){
            critical_error_call(err);
            continue;
        }

        if(res){
            std::cout << "Result:\n" << res.value() << std::endl;
        }
        else{
            std::cout << "Error:\n" << err << std::endl;
        }
    }    
}