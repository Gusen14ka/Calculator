#pragma once
#include<string>
#include <vector>

using size_t = std::size_t;


class Token {
public:
    Token() = default;
    enum Type { NUMBER, FUNC, OP, LPAREN, RPAREN, COMMA } type;
    std::string text;
    std::size_t pos = 0;

    // Фабрики:
    static Token Number(std::string&& s, std::size_t p) { Token t; t.type = NUMBER; t.text = std::move(s); t.pos = p; return t; }
    static Token Func(std::string&& s, std::size_t p) { Token t; t.type = FUNC; t.text = std::move(s); t.pos = p; return t; }
    static Token Op(std::string&& s, std::size_t p) { Token t; t.type = OP; t.text = std::move(s); t.pos = p; return t; }
    static Token LParen(std::size_t p) { Token t; t.type = LPAREN; t.text = "("; t.pos = p; return t; }
    static Token RParen(std::size_t p) { Token t; t.type = RPAREN; t.text = ")"; t.pos = p; return t; }
    static Token Comma(std::size_t p) { Token t; t.type = COMMA; t.text = ","; t.pos = p; return t; }
};

class Lexer{
public:
    explicit Lexer(std::string&& input) : input_(std::move(input)), cur_(0), end_(input_.size()){}
    std::vector<Token> tokenize(std::string& err_out);
    
private:
    // Поля для хранения и обработки входной строки
    std::string input_;
    size_t cur_;
    size_t end_;

    // Пропускаем пробелы до значащего символа
    void skip_spaces();

    // Парсим токен числа из строки
    Token lex_number(std::string& err_out);
};