#include "functional/Executor.hpp"
#include "functional/Parser.hpp"
#include "logger/Logger.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

#define LOG Logger::instance()

void Executor::evaluate(RPN_Number const & numItem){
    stack_.push_back(numItem.val);
}

void Executor::evaluate(RPN_Callable const & callItem, std::string& out_err){
    if(stack_.size() < callItem.arity){
        std::string err;
        out_err = "Stack underflow for operator: " + callItem.ptr->name(&err);
        if(!err.empty()){
            out_err += "Error in callable->name: " + err;   
        }
        LOG.error(out_err, "Executor::evaluate");
        return;
    }

    std::vector<double> args(callItem.arity);
    // Переносим аргументы в обратном порядке
    for(int i = callItem.arity - 1; i >= 0; --i){
        args[i] = stack_.back();
        stack_.pop_back();
    }

    std::string err1, err2, log_err;
    auto res = callItem.ptr->call(args, &err1);
    if(err1.size() != 0){
        out_err = err1;
        log_err = "Error in apllying callable object " + err1 + ": " + callItem.ptr->name(&err2);
        if(!err2.empty()){
            log_err += "Error in callable->name: " + err2;
        }
        LOG.error(log_err, "Executor::evaluate");
        return;
    }

    stack_.push_back(res);
}

void Visitor::operator()(RPN_Number const & numItem){
    executor.evaluate(numItem);
}

void Visitor::operator()(RPN_Callable const & callItem){
    executor.evaluate(callItem, err);
}

std::optional<double> Executor::execute(std::vector<RPN_item> const & items, std::string& out_err){
    std::string err;
    Visitor vis{*this, err};
    
    for(auto const & item : items){
        std::visit(vis, item);
        if(!err.empty()){
            out_err = err;
            return std::nullopt;
        }
    }

    if(stack_.empty()){
        out_err = "empty expression (no result)";
        LOG.warning(out_err, "Executor::execute");
        return std::nullopt;
    }

    if (stack_.size() != 1) {
        out_err = "invalid RPN: leftover operands on stack";
        LOG.error(out_err, "Executor::execute");
        return std::nullopt;
    }

    return stack_.back();
}