#include "functional/Lexer.hpp"
#include "logger/Logger.hpp"
#include <cctype>
#include <string>
#include <vector>

#define LOG Logger::instance()

std::vector<Token> Lexer::tokenize(std::string& err_out){
    std::vector<Token> output;
    output.reserve(64);
    // Если останется 0, значит кол-во открывающих и зкрывающих одинаково
    int parenBalance = 0;
    while(skip_spaces(), cur_ < end_){
        auto u_ch = static_cast<unsigned char>(input_[cur_]);

        // Токен - число, если начинается с цифры или точки(но тогда за ней идёт цифра)
        if(std::isdigit(u_ch) ||
            (u_ch == '.' && cur_ + 1 < end_ && std::isdigit(static_cast<unsigned char>(input_[cur_ + 1]))))
        {
            std::string err;
            auto num = lex_number(err);
            if(!err.empty()){
                err_out = std::move(err);
                return {};
            }
            output.push_back(num);
            continue;
        }

        // Токен - идентификатор(функции), если начинается с буквы, в внутри буквы и '_'
        if(std::isalpha(u_ch)){
            auto pos = cur_;
            ++cur_;
            while(cur_ < end_ && (std::isalpha(static_cast<unsigned char>(input_[cur_])) || input_[cur_] == '_')){
                ++cur_;
            }
            output.emplace_back(Token::Func(input_.substr(pos, cur_ - pos), pos));
            continue;
            // В конце cur_ - индекс следующего символа
        }

        // Все операторы состоят НЕ из букв и цифр
        switch (u_ch) {
            // Операторы - просто добавляем
            case('+'):
            case('-'):
            case('*'):
            case('/'):{
                output.emplace_back(Token::Op(std::string(1, u_ch), cur_));
                ++cur_;
                break;
            }

            // Отдельно добавляем скобки и запятые
            case(','):{
                output.emplace_back(Token::Comma(cur_));
                ++cur_;
                break;
            }
            case('('):{
                output.emplace_back(Token::LParen(cur_));
                ++cur_;
                ++parenBalance;
                break;
            }
            case(')'):{
                output.emplace_back(Token::RParen(cur_));
                ++cur_;
                --parenBalance;
                if(parenBalance < 0){
                    err_out = "The balance of opening and closing parentheses has been disrupted";
                    LOG.error(err_out, "Lexer::tokenize");
                    return {};
                }
                break;
            }
            default:{
                auto pos = cur_;
                ++cur_;
                while(!std::isalnum(static_cast<unsigned char>(input_[cur_])) && static_cast<unsigned char>(input_[cur_]) != ' ' && cur_ < end_){
                    ++cur_;
                }
                output.emplace_back(Token::Op(input_.substr(pos, cur_ - pos), pos));
            }
        }
    }
    if(parenBalance != 0) {
        err_out = "The balance of opening and closing parentheses has been disrupted.";
        LOG.error(err_out, "Lexer::tokenize");
        return {};
    }
    return output;
}

void Lexer::skip_spaces(){
    while(cur_ < end_ && std::isspace(static_cast<unsigned char>(input_[cur_]))){
        ++cur_;
    }
}

Token Lexer::lex_number(std::string& err_out){
    auto pos = cur_;
    bool hasDot = false;
    while(cur_ < end_){
        unsigned char u_ch = input_[cur_];
        // Сначала целая часть числа
        if(isdigit(u_ch)){
            ++cur_;
            continue;
        }
        // Далее Десятичная дробь
        if(u_ch == '.'){
            if(!hasDot){
                ++cur_;
                hasDot = true;
                continue;
            }
            else{
                err_out = "The number recording format is broken. There can only be one decimal point in a number";
                LOG.error(err_out, "Lexer::tokenize");
                return {};
            }
        }
        // Экспоненциальаня часть
        if(u_ch == 'E' || u_ch == 'e'){
            ++cur_;
            if(cur_ < end_){
                if(input_[cur_] == '+' || input_[cur_] == '-'){
                    ++cur_;
                }
            }
            // После знака экспоненты в экспоненциальной форме обязательно должна быть степень
            if(cur_ >= end_ || !std::isdigit(static_cast<unsigned char>(input_[cur_]))){
                err_out = "The number recording format is broken. After the exponent sign, the exponent is expected to be";
                LOG.error(err_out, "Lexer::tokenize");
                return {};
            }
            // Считываем значение степени
            while(cur_ < end_ && std::isdigit(static_cast<unsigned char>(input_[cur_]))){
                ++cur_;
            }
        }
        // Если встретили что-то кроме цифр, точки или e/E заканчиваем
        break;
    }

    std::string text = input_.substr(pos, cur_ - pos);
    return Token::Number(std::move(text), pos);
    // cur_ - после функции - индекс следующего символа за числом
}