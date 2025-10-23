#include "functional/Parser.hpp"
#include "functional/Lexer.hpp"
#include "logger/Logger.hpp"
#include "operators/IOperator.hpp"
#include <charconv>
#include <string>
#include <variant>
#include <vector>

#define LOG Logger::instance()

void Parser::mark_near_func(std::vector<StackItem> & stack, bool& marked_cur_func){
    if(stack.size() == 0) return;
    for(int i = stack.size() - 1; i >= 0; --i){
        if(auto func = std::get_if<FuncItem>(&stack[i])){
            func->has_inside = true;
            marked_cur_func = true;
        }

    }
}

std::vector<RPN_item> Parser::shunting_yard(std::vector<Token> const & tokens, std::string & err_out){
    std::vector<RPN_item> output;
    bool marked_cur_func = false; // Вспомогательный  флаг, чтобы не делать лишних обходов стека
    std::vector<StackItem> stack{};
    auto prev = Token::Type::LPAREN;
    // Обрабатываем каждый токен
    for(unsigned i = 0; i < tokens.size(); ++i){
        auto const & tok = tokens[i];
        
        // В ссответсвии с типом токена, поступаем в соответсвтвии с классической реализацией shunting yard
        switch(tok.type){
            // Токен - число
            case Token::Type::NUMBER: {
                prev = Token::Type::NUMBER;
                std::string err;
                auto num = parseDouble(tok.text, err);
                if(!err.empty()){
                    err_out = std::move(err);
                    return {};
                }
                output.emplace_back(RPN_Number{num});
                if(!marked_cur_func){
                    mark_near_func(stack, marked_cur_func);
                }
                break;
            }

            // Токен - предположительно функция (имя функции)
            case Token::Type::FUNC: {
                bool isFunc = (i + 1 < tokens.size() && tokens[i+1].type == Token::Type::LPAREN);
                if(isFunc){
                    prev = Token::Type::FUNC;
                    stack.emplace_back(FuncItem{tok, tok.text, false, 0});
                    if(!marked_cur_func){
                        mark_near_func(stack, marked_cur_func);
                    }
                    marked_cur_func = false;
                }
                else{
                    err_out = "The syntax of the function entry is broken. An opening parenthesis is expected after the function name";
                    LOG.error(err_out, "Parser::shunting_yard");
                    return{};
                }
                break;
            }

            // Токен - оператор
            case Token::Type::OP: {
                bool isUnary = (prev == Token::OP || prev == Token::LPAREN || prev == Token::COMMA);
                prev = Token::OP;
                unsigned arity = isUnary ? 1 : 2;
                auto opInfo = opReg_.find_operator(tok.text, isUnary);
                if(!opInfo){
                    opInfo = plMg_.find(tok.text);
                    if(!opInfo){
                        err_out = "Syntax error. Unknown operator: " + tok.text;
                        LOG.error(err_out, "Parser::shunting_yard");
                        return{};
                    }
                }

                while(!stack.empty()){
                    if(auto topOp = std::get_if<OpItem>(&stack.back())){
                        auto topInfo = topOp->op;
                        bool goToPop = false;

                        std::string err;
                        
                        // Если следующий оператор имеет больший проиритет или
                        // если текущий оператор лево-ассоциативный и следующий оператор имеет такой же приоритет
                        // то убираем его со стека и пушим в output
                        if(!opInfo->is_right_assoc_operator(&err)){
                            auto a = opInfo->precedence(&err);
                            auto b = topInfo->precedence(&err);
                            if(opInfo->precedence(&err) <= topInfo->precedence(&err)){
                                goToPop = true;
                            }
                        }
                        else{
                            if(opInfo->precedence(&err) < topInfo->precedence(&err)){
                                goToPop = true;
                            }
                        }

                        if(!err.empty()){
                            err_out = err;
                            LOG.error(err, "Parser::shunting_yard");
                            return{};
                        }

                        if(goToPop){
                            std::string err;

                            output.emplace_back(RPN_Callable{topInfo, topOp->arity});
                            stack.pop_back();
                            continue;
                        }
                    }
                    break;
                }
                stack.emplace_back(OpItem{tok, opInfo, arity});
                break;
            }

            // Токен - открывающая(левая) скобка
            case Token::Type::LPAREN: {
                prev = Token::Type::LPAREN;
                stack.push_back(LParenItem{tok});
                break;
            }

            // Токен - запятая
            case Token::Type::COMMA: {
                prev = Token::Type::COMMA;
                while(!stack.empty()){
                    if(auto topOp = std::get_if<OpItem>(&stack.back())){
                        output.emplace_back(RPN_Callable{topOp->op, topOp->arity});
                        stack.pop_back();
                    }
                    else{
                        break;
                    }
                }
                // В стеке гарантированно должна всё ещё быть открывающая скобка и функция
                if(stack.empty() || !std::holds_alternative<LParenItem>(stack.back())){
                    err_out = "Syntax error. An unexpected comma";
                    LOG.error(err_out, "Parser::shunting_yard");
                    return {};
                }
                int idxLP = stack.size() - 1;
                bool found = false;
                for (int k = idxLP - 1; k >= 0; --k) {
                    if (auto pf = std::get_if<FuncItem>(&stack[k])){ 
                        pf->comma_count += 1; 
                        found = true; 
                        break;
                    }
                    if (std::holds_alternative<LParenItem>(stack[k])){
                        break; // встретили внешнюю '(' — значит это не функция
                    }
                }
                if (!found) {
                    err_out = "Syntax error. An unexpected comma";
                    LOG.error(err_out, "Parser::shunting_yard");
                    return {};
                }
                break;
            }

            // Токен - Закрывающая (правая) скобка
            case Token::Type::RPAREN: {
                prev = Token::Type::RPAREN;
                while(!stack.empty()){
                    if(auto topOp = std::get_if<OpItem>(&stack.back())){
                        output.emplace_back(RPN_Callable{topOp->op, topOp->arity});
                        stack.pop_back();
                    }
                    else{
                        break;
                    }
                }
                
                // В стеке гарантированно должна всё ещё быть открывающая скобка
                if(stack.empty() || !std::holds_alternative<LParenItem>(stack.back())){
                    err_out = "The balance of opening and closing parentheses has been disrupted";
                    LOG.error(err_out, "Parser::shunting_yard");
                    return {};
                }

                stack.pop_back(); // Убрали открывающую скобку из стека

                //Если наверху осталась функция - отправляем её в output
                if(!stack.empty() && std::holds_alternative<FuncItem>(stack.back())){
                    auto func = std::get<FuncItem>(stack.back());
                    stack.pop_back();
                    unsigned argc = func.has_inside ? (func.comma_count + 1) : 0;
                    auto callable = plMg_.find(func.name);
                    if(!callable){
                        err_out = "Syntax error. Unknown function: " + func.name;
                        LOG.error(err_out, "Parser::shunting_yard");
                        return {};
                    }
                    output.emplace_back(RPN_Callable{callable, argc});
                    marked_cur_func = false;
                }
            }
        }
    }

    // Если что-то осталось обрабатываем
    while(!stack.empty()){
        if(std::holds_alternative<LParenItem>(stack.back())){
            err_out = "Syntax error. An unexpected opening parenthesis";
            LOG.error(err_out, "Parser::shunting_yard");
            return {};
        }
        if(auto topOp = std::get_if<OpItem>(&stack.back())){
            output.emplace_back(RPN_Callable{topOp->op, topOp->arity});
            stack.pop_back();
        }
        else{
            err_out = "Syntax error...";
            LOG.error(err_out, "Parser::shunting_yard");
        }
    }

    return output;

}

double Parser::parseDouble(std::string const & str, std::string & err_out){
    const char* first = str.data();
    const char* last  = str.data() + str.size();
    double val = 0;

    auto [ptr, ec] = std::from_chars(first, last, val, std::chars_format::general);

    if (ec == std::errc()) {
        // Успешно, но проверим, что вся строка была съедена
        if (ptr == last) {
            return val; // всё хорошо
        } else {
            err_out = "Error in std::from_chars: " + str;
            LOG.error(err_out, "Parser::parseDouble");
            return{};
        }
    }

    if (ec == std::errc::invalid_argument) {
        err_out = "Error in std::from_chars: invalid_argument: "  + str;
    } else if (ec == std::errc::result_out_of_range) {
        err_out = "Error in std::from_chars: invalid_argument: "  + str;
    }
    if(err_out.empty()) err_out = "Error in std::from_chars: " + str;
    LOG.error(err_out, "Parser::parseDouble");
    return{};
}