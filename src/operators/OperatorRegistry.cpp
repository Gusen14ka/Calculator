#include "operators/OperatorRegistry.hpp"

OperatorRegistry::OperatorRegistry() {
    operators_.reserve(5);
}

void OperatorRegistry::register_operator(std::shared_ptr<ICallable> op, bool unary) {
    std::string err;
    if (op) {
        operators_[key(op->name(&err), unary)] = op;
    }
}

std::shared_ptr<ICallable> const OperatorRegistry::find_operator(std::string const & symbol, bool unary) const {
    auto it = operators_.find(key(symbol, unary));
    return it != operators_.end() ? it->second : nullptr;
}

std::string OperatorRegistry::key(const std::string &symbol, bool unary){
    return symbol + (unary ? "#u" : "#b"); 
}
