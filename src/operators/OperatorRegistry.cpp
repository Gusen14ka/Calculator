#include "operators/OperatorRegistry.hpp"
#include "operators/IOperator.hpp"

OperatorRegistry::OperatorRegistry() {
    operators_.reserve(5);
}

void OperatorRegistry::register_operator(std::shared_ptr<IOperator> op, bool unary) {
    if (op) {
        operators_[key(op->name(), unary)] = op;
    }
}

std::shared_ptr<IOperator> const OperatorRegistry::find_operator(std::string const & symbol, bool unary) const {
    auto it = operators_.find(symbol);
    return it != operators_.end() ? it->second : nullptr;
}

std::string OperatorRegistry::key(const std::string &symbol, bool unary){
    return symbol + (unary ? "#u" : "#b"); 
}
