#pragma once
#include "IOperator.hpp"
#include <vector>

class AddOperator : public IOperator{
public:
    std::string name() const override { return "+"; }
    Op_precedence precedence() const override { return Op_precedence::THIRD; }
    bool is_right_assoc() const override { return false; }

    double apply(std::vector<double> const & args, std::string * err_out) const override {
        if(args.size() != 2){
            //TODO: LOG.error
            if(err_out) *err_out = "expected 2 args";
        }
        return args[0] + args[1];
    }
};