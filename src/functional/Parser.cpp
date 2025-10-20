#include "functional/Parser.hpp"
#include <charconv>
#include <string>
#include <variant>
#include <vector>

void Parser::mark_near_func(std::vector<StackItem> & stack, bool& marked_cur_func){
    if(stack.size() == 0) return;
    for(int i = stack.size() - 1; i >= 0; --i){
        if(auto func = std::get_if<FuncItem>(&stack[i])){
            func->has_inside = true;
            marked_cur_func = true;
        }

    }
}

std::vector<RPN_item> Parser::shunting_yard(std::vector<Token> const & tokens){
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
                output.emplace_back(RPN_Number{parseDouble(tok.text)});
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
                    // TODO: обработка исключения
                    throw;
                }
                break;
            }

            // Токен - оператор
            case Token::Type::OP: {
                bool isUnary = (prev == Token::OP || prev == Token::LPAREN || prev == Token::COMMA);
                unsigned arity = isUnary ? 1 : 2;
                auto opInfo = opReg_.find_operator(tok.text, isUnary);
                if(!opInfo){
                    opInfo = plMg_.find(tok.text);
                    if(!opInfo){
                        //TODO LOG.error
                        throw;
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
                            if(opInfo->precedence(&err) == topInfo->precedence(&err)){
                                goToPop = true;
                            }
                        }
                        else{
                            if(opInfo->precedence(&err) < topInfo->precedence(&err)){
                                goToPop = true;
                            }
                        }

                        if(err != ""){
                            //TODO: LOG.error
                            throw;
                        }

                        if(goToPop){
                            output.emplace_back(RPN_Callable{topInfo, arity});
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
                    // TODO: обработка исключения
                    throw;
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
                    // TODO: обработка исключения
                    throw;
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
                    // TODO: обработка исключения
                    throw;
                }

                stack.pop_back(); // Убрали открывающую скобку из стека

                //Если наверху осталась функция - отправляем её в output
                if(!stack.empty() && std::holds_alternative<FuncItem>(stack.back())){
                    auto func = std::get<FuncItem>(stack.back());
                    stack.pop_back();
                    unsigned argc = func.has_inside ? (func.comma_count + 1) : 0;
                    auto callable = plMg_.find(func.name);
                    if(!callable){
                        //TODO: LOG.error
                        throw;
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
            // TODO: обработка исключения
            throw;
        }
        if(auto topOp = std::get_if<OpItem>(&stack.back())){
            output.emplace_back(RPN_Callable{topOp->op, topOp->arity});
            stack.pop_back();
        }
        else{
            // TODO: обработка исключения
            throw;
        }
    }

    return output;

}

double Parser::parseDouble(std::string const & str){
    const char* first = str.data();
    const char* last  = str.data() + str.size();
    double val = 0;

    auto [ptr, ec] = std::from_chars(first, last, val, std::chars_format::general);

    if (ec == std::errc()) {
        // Успешно, но проверим, что вся строка была съедена
        if (ptr == last) {
            return val; // всё хорошо
        } else {
            // TODO:
            //std::cerr << "Trailing garbage after number: '" << std::string(ptr, last) << "'\n";
            throw;
        }
    }

    if (ec == std::errc::invalid_argument) {
        // TODO:
        // std::cerr << "Invalid number: '" << str << "'\n";
    } else if (ec == std::errc::result_out_of_range) {
        // TODO:
        //std::cerr << "Number out of range: '" << str << "'\n";
    }
    throw;
}