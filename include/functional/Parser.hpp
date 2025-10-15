#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "operators/IOperator.hpp"
#include "operators/OperatorRegistry.hpp"
#include "Lexer.hpp"

// Элементы стека в алгоритме shunting-yard
struct OpItem{
    Token tok;
    std::shared_ptr<IOperator> op;
};
struct FuncItem{
    Token tok;
    std::string name;
    bool has_inside = false;
    int comma_count = 0;
};
struct LParenItem{
    Token tok;
};

using StackItem = std::variant<OpItem, FuncItem, LParenItem>;



// Элементы обратной польской нотации:
struct RPN_Number { double val; };
struct RPN_Op { std::shared_ptr<IOperator> op; };
struct RPN_Func { std::string name; unsigned arity; };
using RPN_item = std::variant<RPN_Number, RPN_Op, RPN_Func>;

class Parser{
public:
    Parser(OperatorRegistry const & opReg) : opReg_(opReg) {}

    std::vector<RPN_item> shunting_yard(std::vector<Token> const& tokens);
    
private:
    // Помечаем функцию, что у неё есть аргументы
    static void mark_near_func(std::vector<StackItem> & stack, bool& marked_cur_func);

    // Парсинг строки-токена в число
    static double parseDouble(std::string const & str);

    const OperatorRegistry opReg_;
};