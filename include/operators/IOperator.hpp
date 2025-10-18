#pragma once
#include <vector>
#include <string>

enum class Op_precedence {ZERO, FIRST, SECOND, THIRD};

class IOperator{
public:
    virtual Op_precedence precedence() const = 0;
    virtual bool is_right_assoc() const = 0; // true если a ^ b ^ c = a ^ (b ^ c)
    virtual double apply(std::vector<double> const & args, std::string * err_out) const = 0;
    virtual std::string name() const = 0;
    virtual unsigned arity() const = 0;
    virtual ~IOperator() = default;
};