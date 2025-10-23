#include "operators/OperatorRegistry.hpp"
#include "ICallable.hpp"
#include "operators/AddOperator.hpp"
#include "operators/DivOperator.hpp"
#include "operators/MulOperator.hpp"
#include "operators/SubOperator.hpp"
#include "operators/UnarAddOperator.hpp"
#include <memory>

OperatorRegistry::OperatorRegistry() {
    operators_.reserve(5);
}

void OperatorRegistry::register_builtin_operators() {
    std::string err;

    auto addOp = std::make_shared<AddOperator>();
    auto unarAddOp = std::make_shared<UnarAddOperator>();
    auto divOp = std::make_shared<DivOperator>();
    auto mulOp = std::make_shared<MulOperator>();
    auto subOp = std::make_shared<SubOperator>();

    operators_[ key(addOp->name(&err), false) ] = addOp;
    operators_[ key(unarAddOp->name(&err), true) ] = unarAddOp;
    operators_[ key(divOp->name(&err), false) ] = divOp;
    operators_[ key(mulOp->name(&err), false) ] = mulOp;
    operators_[ key(subOp->name(&err), false) ] = subOp;
}

std::shared_ptr<ICallable> const OperatorRegistry::find_operator(std::string const & symbol, bool unary) const {
    auto it = operators_.find(key(symbol, unary));
    return it != operators_.end() ? it->second : nullptr;
}

std::string OperatorRegistry::key(const std::string &symbol, bool unary){
    return symbol + (unary ? "#u" : "#b"); 
}