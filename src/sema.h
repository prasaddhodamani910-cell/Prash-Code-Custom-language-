#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "ast.h"
#include "error.h"

namespace toy {

class Sema {
public:
    Sema(ErrorReporter& reporter);
    void analyze(Program* program);

private:
    void visitFnDecl(FnDecl* fn);
    void visitBlock(Block* block);
    void visitStmt(Stmt* stmt);
    void visitExpr(Expr* expr);
    
    void enterScope();
    void leaveScope();
    void declareVariable(const std::string& name, Location loc);
    void resolveVariable(const std::string& name, Location loc);

    ErrorReporter& m_reporter;
    std::vector<std::unordered_set<std::string>> m_scopes;
    std::vector<std::unordered_map<std::string, int>> m_arraySizes;
    std::unordered_map<std::string, int> m_functions; // name -> param count
    bool m_inFunction = false;
};

} // namespace toy
