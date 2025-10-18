#include "functional/Executor.hpp"
#include "functional/Parser.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

void Executor::evaluate(RPN_Number const & numItem, std::string * out_err){
    stack_.push_back(numItem.val);
}

void Executor::evaluate(RPN_Op const & opItem, std::string * out_err){
    if(!opItem.op){
        if(out_err) *out_err = "op is null operator";
        //TODO: LOG.error
        return;
    }

    auto argc = opItem.op->arity();
    if(stack_.size() < argc){
        if(out_err) *out_err = "stack underflow for operator";
        //TODO: LOG.error
        return;
    }

    std::vector<double> args(argc);
    // Переносим аргументы в обратном порядке
    for(unsigned i = argc - 1; i >= 0; --i){
        args[i] = stack_.back();
        stack_.pop_back();
    }

    std::string err;
    auto res = opItem.op->apply(args, &err);
    if(err.size()){
        //TODO: LOG.error
        if(out_err) *out_err = "Error in apllying operator " + err;
        return;
    }

    stack_.push_back(res);
}

void Executor::evaluate(RPN_Func const & funcItem, std::string * out_err){
    auto plug_handle = plug_manager_.find(funcItem.name);
    if(!plug_handle){
        //TODO: LOG.error
        if(out_err) *out_err = "No such function: " + funcItem.name;
        return;
    }

    auto argc = funcItem.arity;
    if(stack_.size() < argc){
        //TODO: LOG.error
        if(out_err) *out_err = "stack underflow for function: " + funcItem.name;
        return;
    }

    std::vector<double> args(argc);
    for(unsigned i = argc - 1; i >= 0; --i){
        args[i] = stack_.back();
        stack_.pop_back();
    }

    // Вызываем функцию из длл
    if(!plug_handle->func){
        //TODO: LOG.error
        if(out_err) *out_err = "plugin function pointer is null for: " + funcItem.name;
        return;
    }

    // Подготавливаем аргументы
    int err_code = 0;
    char err_msg[ERROR_BUF_SIZE] = {0};
    double res = 0.0;
    
    // Вообще подразумевается, что плагины не будут кидать исключения, но для безопасности обернём в try
    try{
        res = plug_handle->func(argc,
            args.data(),
            &err_code,
            err_msg,
            sizeof(err_msg));
    }
    catch (const std::exception& e) {
        //TODO: LOG.error
        if(out_err) *out_err = "plugin threw exception: " + std::string(e.what());
        if(out_err && err_code != 0) *out_err += " Error code: " + std::to_string(err_code);
        if(out_err && err_msg[0] != '0') *out_err += " Error message: " + std::string(err_msg);
        return;
    } catch (...) {
        //TODO: LOG.error
        if(out_err) *out_err = "plugin threw unknown exception from function: " + funcItem.name;
        if(out_err && err_code != 0) *out_err += " Error code: " + std::to_string(err_code);
        if(out_err && err_msg[0] != '0') *out_err += " Error message: " + std::string(err_msg);
        return;
    }
    if(err_code != 0){
        //TODO: LOG.error
        if(out_err && err_code != 0) *out_err =  "Plugin returned error Error code: " + std::to_string(err_code);
        if(out_err && err_msg[0] != '0') *out_err += " Error message: " + std::string(err_msg);
        return;
    }

    stack_.push_back(res);
}

void Visitor::operator()(RPN_Number const & numItem){
    executor.evaluate(numItem, err);
}

void Visitor::operator()(RPN_Op const & opItem){
    executor.evaluate(opItem, err);
}

void Visitor::operator()(RPN_Func const & functItem){
    executor.evaluate(functItem, err);
}

std::optional<double> Executor::execute(std::vector<RPN_item> const & items, std::string* out_err){
    std::string err;
    Visitor vis{*this, &err};
    
    for(auto const & item : items){
        std::visit(vis, item);
        if(!err.empty()){
            //TODO: LOG.error а может и не надо выносить (дублировать) на этот уровнь
            if(out_err) *out_err = "Error in executor: " + err;
            return std::nullopt;
        }
    }

    if(stack_.empty()){
        //TODO: LOG.error
        if (out_err) *out_err = "empty expression (no result)";
        return std::nullopt;
    }

    if (stack_.size() != 1) {
        //TODO: LOG.error
        if (out_err) *out_err = "invalid RPN: leftover operands on stack";
        return std::nullopt;
    }

    return stack_.back();
}