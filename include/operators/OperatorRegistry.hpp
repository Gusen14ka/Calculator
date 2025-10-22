#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "ICallable.hpp"


class OperatorRegistry {
public:
    OperatorRegistry();

    // Добавление всех встроенных операторов
    void register_builtin_operators();

    // Поиск оператора по его символу
    std::shared_ptr<ICallable> const find_operator(std::string const & symbol, bool unary) const;

private:
    static std::string key(const std::string &symbol, bool unary);
    // ключ мапы - "символ#(b/u)" b - если оператор бинарный, u - унарный
    std::unordered_map<std::string, std::shared_ptr<ICallable>> operators_; 
};