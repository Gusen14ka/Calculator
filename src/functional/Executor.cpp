#include "functional/Executor.hpp"
#include "functional/Parser.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

void Executor::evaluate(RPN_Number const & numItem, std::string * out_err){
    stack_.push_back(numItem.val);
}

void Executor::evaluate(RPN_Callable const & callItem, std::string * out_err){
    if(stack_.size() < callItem.arity){
        if(out_err) *out_err = "stack underflow for operator";
        //TODO: LOG.error
        return;
    }

    std::vector<double> args(callItem.arity);
    // Переносим аргументы в обратном порядке
    for(unsigned i = callItem.arity - 1; i >= 0; --i){
        args[i] = stack_.back();
        stack_.pop_back();
    }

    std::string err;
    auto res = callItem.op->call(args, &err);
    if(err.size() != 0){
        //TODO: LOG.error
        if(out_err) *out_err = "Error in apllying callable object " + err;
        return;
    }

    stack_.push_back(res);
}

void Visitor::operator()(RPN_Number const & numItem){
    executor.evaluate(numItem, err);
}

void Visitor::operator()(RPN_Callable const & callItem){
    executor.evaluate(callItem, err);
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