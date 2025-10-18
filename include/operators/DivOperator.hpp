#pragma once
#include "IOperator.hpp"
#include <vector>

class DivOperator : public IOperator{
public:
    std::string name() const override { return "/"; }
    Op_precedence precedence() const override { return Op_precedence::SECOND; }
    bool is_right_assoc() const override { return false; }

    double apply(std::vector<double> const & args, std::string * err_out) const override {
        if(args.size() != 2){
            //TODO: LOG.error
            if(err_out) *err_out = "expected 2 args";
        }
        if(args[1] == 0){
            //TODO: LOG.error
            if(err_out) *err_out = "division by zero";
            return 0.0;
        }
        return args[0] / args[1];
    }
};