#include "sema.h"

namespace toy {

Sema::Sema(ErrorReporter& reporter) : m_reporter(reporter) {}

void Sema::analyze(Program* program) {
    for (const auto& fn : program->functions) {
        if (m_functions.count(fn->name)) {
            m_reporter.error(fn->loc, "Function '" + fn->name + "' already defined");
        } else {
            m_functions[fn->name] = fn->params.size();
        }
    }
    
    m_functions["print"] = 1;

    for (const auto& fn : program->functions) {
        visitFnDecl(fn.get());
    }
}

void Sema::visitFnDecl(FnDecl* fn) {
    m_inFunction = true;
    enterScope();
    for (const auto& param : fn->params) {
        declareVariable(param, fn->loc);
    }
    visitBlock(fn->body.get());
    leaveScope();
    m_inFunction = false;
}

void Sema::visitBlock(Block* block) {
    enterScope();
    for (const auto& stmt : block->statements) {
        visitStmt(stmt.get());
    }
    leaveScope();
}

void Sema::visitStmt(Stmt* stmt) {
    if (auto assignStmt = dynamic_cast<AssignStmt*>(stmt)) {
        visitExpr(assignStmt->value.get());
        
        bool found = false;
        int existingArraySize = 0;
        bool isExistingArray = false;
        for (size_t i = m_scopes.size(); i-- > 0;) {
            if (m_scopes[i].count(assignStmt->name)) {
                found = true;
                if (m_arraySizes[i].count(assignStmt->name)) {
                    isExistingArray = true;
                    existingArraySize = m_arraySizes[i][assignStmt->name];
                }
                break;
            }
        }
        
        auto arrLit = dynamic_cast<ArrayLiteral*>(assignStmt->value.get());
        bool isNewArray = arrLit != nullptr;
        
        if (!found) {
            declareVariable(assignStmt->name, assignStmt->loc);
            assignStmt->isDeclaration = true;
            if (isNewArray) {
                m_arraySizes.back()[assignStmt->name] = arrLit->elements.size();
            }
        } else {
            assignStmt->isDeclaration = false;
            if (isNewArray && !isExistingArray) {
                m_reporter.error(assignStmt->loc, "Cannot assign array to scalar variable '" + assignStmt->name + "'");
            } else if (!isNewArray && isExistingArray) {
                m_reporter.error(assignStmt->loc, "Cannot assign scalar to array variable '" + assignStmt->name + "'");
            } else if (isNewArray && isExistingArray) {
                if (existingArraySize != (int)arrLit->elements.size()) {
                    m_reporter.error(assignStmt->loc, "Cannot reassign array '" + assignStmt->name + "' to different size");
                }
            }
        }
    } else if (auto idxAssign = dynamic_cast<IndexAssignStmt*>(stmt)) {
        visitExpr(idxAssign->array.get());
        visitExpr(idxAssign->index.get());
        visitExpr(idxAssign->value.get());
    } else if (auto ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        visitExpr(ifStmt->condition.get());
        visitStmt(ifStmt->thenBranch.get());
        if (ifStmt->elseBranch) {
            visitStmt(ifStmt->elseBranch.get());
        }
    } else if (auto whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        visitExpr(whileStmt->condition.get());
        visitStmt(whileStmt->body.get());
    } else if (auto retStmt = dynamic_cast<ReturnStmt*>(stmt)) {
        if (!m_inFunction) {
            m_reporter.error(retStmt->loc, "Return statement outside of function");
        }
        if (retStmt->value) {
            visitExpr(retStmt->value.get());
        }
    } else if (auto printStmt = dynamic_cast<PrintStmt*>(stmt)) {
        visitExpr(printStmt->value.get());
    } else if (auto printStrStmt = dynamic_cast<PrintStrStmt*>(stmt)) {
        visitExpr(printStrStmt->value.get());
    } else if (auto exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        visitExpr(exprStmt->expr.get());
    } else if (auto blockStmt = dynamic_cast<Block*>(stmt)) {
        visitBlock(blockStmt);
    }
}

void Sema::visitExpr(Expr* expr) {
    if (auto binExpr = dynamic_cast<BinaryExpr*>(expr)) {
        visitExpr(binExpr->left.get());
        visitExpr(binExpr->right.get());
    } else if (auto unExpr = dynamic_cast<UnaryExpr*>(expr)) {
        visitExpr(unExpr->right.get());
    } else if (auto callExpr = dynamic_cast<CallExpr*>(expr)) {
        if (!m_functions.count(callExpr->callee)) {
            m_reporter.error(callExpr->loc, "Undefined function '" + callExpr->callee + "'");
        } else if (m_functions[callExpr->callee] != (int)callExpr->args.size()) {
            m_reporter.error(callExpr->loc, "Incorrect number of arguments for function '" + callExpr->callee + "'");
        }
        for (const auto& arg : callExpr->args) {
            visitExpr(arg.get());
        }
    } else if (auto idExpr = dynamic_cast<Identifier*>(expr)) {
        resolveVariable(idExpr->name, idExpr->loc);
    } else if (auto arrLit = dynamic_cast<ArrayLiteral*>(expr)) {
        for (const auto& el : arrLit->elements) visitExpr(el.get());
    } else if (auto idxExpr = dynamic_cast<IndexExpr*>(expr)) {
        visitExpr(idxExpr->array.get());
        visitExpr(idxExpr->index.get());
    }
}

void Sema::enterScope() {
    m_scopes.push_back({});
    m_arraySizes.push_back({});
}

void Sema::leaveScope() {
    m_scopes.pop_back();
    m_arraySizes.pop_back();
}

void Sema::declareVariable(const std::string& name, Location loc) {
    if (m_scopes.empty()) return;
    auto& scope = m_scopes.back();
    if (scope.count(name)) {
        m_reporter.error(loc, "Variable '" + name + "' already declared in this scope");
    } else {
        scope.insert(name);
    }
}

void Sema::resolveVariable(const std::string& name, Location loc) {
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        if (it->count(name)) {
            return;
        }
    }
    m_reporter.error(loc, "Undeclared variable '" + name + "'");
}

} // namespace toy
