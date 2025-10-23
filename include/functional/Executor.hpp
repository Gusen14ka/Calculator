#pragma once
#include "functional/Parser.hpp"
#include <optional>
#include <vector>


class Executor{
public:
    Executor() { stack_.reserve(64); }

    std::optional<double> execute(std::vector<RPN_item> const & items, std::string& out_err);
private:
    std::vector<double> stack_;

    // Перегрузки функций для каждого типа RPN последовательности
    void evaluate(RPN_Number const & numItem);
    void evaluate(RPN_Callable const & callItem, std::string& out_err);

    // Обёртка для использование std::visit
    friend struct Visitor;
};

struct Visitor{
    Executor& executor;
    std::string& err;
    void operator()(RPN_Number const & numItem);
    void operator()(RPN_Callable const & callItem);
};