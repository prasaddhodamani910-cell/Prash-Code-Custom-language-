#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "ast.h"

namespace toy {

class CodeGen {
public:
    CodeGen(const std::string& outputFile);
    void generate(Program* program);

private:
    void generateFnDecl(FnDecl* fn);
    void generateBlock(Block* block);
    void generateStmt(Stmt* stmt);
    void generateExpr(Expr* expr);
    
    // Output helpers
    void emit(const std::string& instr);
    void emitLabel(const std::string& label);
    
    // Stack and scope management
    void push(const std::string& reg = "x0");
    void pop(const std::string& reg = "x0");
    void enterScope();
    void leaveScope();
    int getVarOffset(const std::string& name);
    void declareVar(const std::string& name);

    int m_labelCounter = 0;
    std::string newLabel();
    
    std::string m_outputFile;
    std::ofstream m_out;
    
    struct VarInfo {
        int offset; // offset from FP
        int scopeDepth;
    };
    std::unordered_map<std::string, std::vector<VarInfo>> m_variables;
    
    struct Scope {
        int numVars = 0;
    };
    std::vector<Scope> m_scopes;
    
    int m_scopeDepth = 0;
    int m_currentFpOffset = 0;
    std::string m_currentFunction;
};

} // namespace toy
