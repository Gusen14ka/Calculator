#pragma once
#include "IOperator.hpp"
#include <vector>

class DivOperator : public IOperator{
public:
    std::string name() const override { return "/"; }
    Op_precedence precedence() const override { return Op_precedence::SECOND; }
    bool is_right_assoc() const override { return false; }

    double apply(std::vector<double> const & args) const override {
        if(args.size() != 2){
            //TODO: обработка исключений
            throw;
        }
        return args[0] / args[1];
    }
};