#pragma once
#include "ICallable.hpp"
#include "logger/Logger.hpp"
#include <vector>

#define LOG Logger::instance()

class DivOperator : public ICallable{
public:
    DivOperator() = default;
    std::string name(std::string* err_out) const override { return "/"; }
    Precedence precedence(std::string* err_out) const override { return Precedence::FIRST; }
    bool is_right_assoc_operator(std::string* err_out) const override { return false; }
    std::pair<int, int> arity(std::string* err_out) const override { return {2, 2}; }
    double call(std::vector<double> const & args, std::string * err_out) override {
        if(args.size() != 2){
            if(err_out) *err_out = "Expected 2 args";
            LOG.error("Expected 2 arguments, get: " + std::to_string(args.size()), "DivOperator::call");
            return {};
        }
        if(args[1] == 0){
            if(err_out) *err_out = "Division by zero";
            LOG.error("Division by zero", "DivOperator::call");
            return {};
        }
        return args[0] / args[1];
    }
};