#include "functional/Lexer.hpp"
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>


std::vector<Token> Lexer::tokenize(){
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
            output.push_back(lex_number());
            continue;
        }

        // Токен - идентификатор(функции), если начинается с буквы, в внутри буквы, цифры и '_'
        if(std::isalpha(u_ch)){
            auto pos = cur_;
            ++cur_;
            while(cur_ < end_ && (std::isalnum(static_cast<unsigned char>(input_[cur_])) || input_[cur_] == '_')){
                ++cur_;
            }
            output.emplace_back(Token::Ident(input_.substr(pos, cur_ - pos), pos));
            continue;
            // В конце cur_ - индекс следующего символа
        }

        switch (u_ch) {
            // Операторы - просто добавляем
            case('+'):
            case('-'):
            case('*'):
            case('/'):
            case('^'):{
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
                    //TODO:
                    throw;
                }
                break;
            }
            default:{
                //TODO:
                throw std::runtime_error("Unexpected symbol at" + std::to_string(cur_));
            }
        }

    }
    if(parenBalance != 0) {
        //TODO:
        throw;
    }
    return output;
}

void Lexer::skip_spaces(){
    while(cur_ < end_ && std::isspace(static_cast<unsigned char>(input_[cur_]))){
        ++cur_;
    }
}

Token Lexer::lex_number(){
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
                //TODO:
                throw;
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
                //TODO:
                throw;
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