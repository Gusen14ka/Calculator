#include "functional/Executor.hpp"
#include "functional/Parser.hpp"
#include "include/operators/AddOperator.hpp"
#include "include/operators/OperatorRegistry.hpp"
#include "plugin_helpers/PluginManager.hpp"
#include "functional/Lexer.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <iostream>

int main(){
    std::cout << "Enter math expression:\n";
    std::string input;
    std::getline(std::cin, input);
    std::string err;

    std::filesystem::path plugin_dir = "D:\\Dev\\VScode\\CPP\\Calculator\\plugins";

    PluginManager plugin_manage(nullptr);
    plugin_manage.scan_directory(plugin_dir);

    OperatorRegistry op_reg;
    std::shared_ptr<AddOperator> addOp = std::make_shared<AddOperator>();

    op_reg.register_operator(addOp, false);

    Lexer lex(std::move(input));
    Parser parser(op_reg, plugin_manage);
    Executor executor;

    auto tokens = lex.tokenize();
    auto items = parser.shunting_yard(tokens);
    auto res = executor.execute(items, &err);

    if(res){
        std::cout << "Result:\n" << res.value() << std::endl;
    }
    else{
        std::cout << "Error:\n" << err << std::endl;
    }
}