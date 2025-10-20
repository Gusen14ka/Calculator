#pragma once
#include "ICallable.hpp"
#include <vector>

class SubOperator : public ICallable{
public:
    std::string name(std::string* err_out) const override { return "-"; }
    Precedence precedence(std::string* err_out) const override { return Precedence::THIRD; }
    bool is_right_assoc_operator(std::string* err_out) const override { return false; }
    std::pair<int, int> arity(std::string* err_out) const override { return {2, 2}; }
    double call(std::vector<double> const & args, std::string * err_out) override {
        if(args.size() != 2){
            //TODO: LOG.error
            if(err_out) *err_out = "expected 2 args";
        }
        return args[0] - args[1];
    }
};