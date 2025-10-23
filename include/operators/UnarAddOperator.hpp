#pragma once
#include "ICallable.hpp"
#include "logger/Logger.hpp"
#include <string>
#include <vector>

#define LOG Logger::instance()
class UnarAddOperator : public ICallable{
public:
    UnarAddOperator() = default;
    std::string name(std::string* err_out) const override { return "+"; }
    Precedence precedence(std::string* err_out) const override { return Precedence::THIRD; }
    bool is_right_assoc_operator(std::string* err_out) const override { return true; }
    std::pair<int, int> arity(std::string* err_out) const override { return {2, 2}; }
    double call(std::vector<double> const & args, std::string * err_out) override {
        if(args.size() != 1){
            if(err_out) *err_out = "Expected 1 args";
            LOG.error("Expected 1 arguments, get: " + std::to_string(args.size()), "UnarAddOperator::call");
            return {};
        }
        return args[0];
    }
};