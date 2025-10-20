#pragma once

#include <string>
#include <vector>

enum class Precedence {ZERO, FIRST, SECOND, THIRD};

class ICallable{
public:
    virtual std::string name(std::string* err_out) const = 0;
    
    // Возвращает пару {минимальное число аргументов, максимальное}
    // Если фозвращает { ,-1} => с переменным числом аргументов (для функций)
    virtual std::pair<int, int> arity(std::string* err_out) const = 0;
    
    virtual Precedence precedence(std::string* err_out) const = 0;
    virtual bool is_right_assoc_operator(std::string* err_out) const = 0;
    virtual double call(std::vector<double> const & args, std::string* err_out);
    virtual ~ICallable() = default;
};