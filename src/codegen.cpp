#include "codegen.h"
#include <iostream>

namespace toy {

CodeGen::CodeGen(const std::string& outputFile) : m_outputFile(outputFile) {
    m_out.open(outputFile);
}

void CodeGen::emit(const std::string& instr) {
    m_out << "    " << instr << "\n";
}

void CodeGen::emitLabel(const std::string& label) {
    m_out << label << ":\n";
}

std::string CodeGen::newLabel() {
    return ".L" + std::to_string(m_labelCounter++);
}

void CodeGen::push(const std::string& reg) {
    emit("str " + reg + ", [sp, #-16]!");
}

void CodeGen::pop(const std::string& reg) {
    emit("ldr " + reg + ", [sp], #16");
}

void CodeGen::enterScope() {
    m_scopes.push_back({0});
    m_scopeDepth++;
}

void CodeGen::leaveScope() {
    int varsToPop = m_scopes.back().numVars;
    if (varsToPop > 0) {
        emit("add sp, sp, #" + std::to_string(varsToPop * 16));
        m_currentFpOffset += varsToPop * 16;
    }
    
    // Remove variables from maps
    for (auto& pair : m_variables) {
        if (!pair.second.empty() && pair.second.back().scopeDepth == m_scopeDepth) {
            pair.second.pop_back();
        }
    }
    
    m_scopes.pop_back();
    m_scopeDepth--;
}

void CodeGen::declareVar(const std::string& name) {
    m_currentFpOffset -= 16;
    m_variables[name].push_back({m_currentFpOffset, m_scopeDepth});
    m_scopes.back().numVars++;
}

int CodeGen::getVarOffset(const std::string& name) {
    auto it = m_variables.find(name);
    if (it != m_variables.end() && !it->second.empty()) {
        return it->second.back().offset;
    }
    return 0; // Should be caught by semantic analyzer
}

void CodeGen::generate(Program* program) {
    m_out << ".arch armv8-a\n";
    m_out << ".text\n";
    m_out << ".align 2\n";
    m_out << ".global _start\n\n";
    
    // _start routine
    emitLabel("_start");
    emit("bl main");
    emit("mov x8, #93"); // exit syscall
    // result of main is in x0
    emit("svc #0");
    m_out << "\n";

    // print runtime
    emitLabel("_print_int");
    // arg is in x0
    // print integer and newline
    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    emit("sub sp, sp, #32"); // buffer for string
    
    emit("mov x1, sp");
    emit("add x1, x1, #30"); // end of buffer
    emit("mov w2, #10");
    emit("strb w2, [x1]"); // newline
    
    emit("mov x3, x0"); // number
    emit("cmp x3, #0");
    emit("bge .Lprint_pos");
    emit("neg x3, x3"); // absolute value
    emitLabel(".Lprint_pos");
    
    emit("mov x4, #10"); // divisor
    emitLabel(".Lprint_loop");
    emit("sub x1, x1, #1");
    emit("udiv x5, x3, x4");
    emit("msub x6, x5, x4, x3"); // remainder
    emit("add x6, x6, #'0'");
    emit("strb w6, [x1]");
    emit("mov x3, x5");
    emit("cmp x3, #0");
    emit("bne .Lprint_loop");
    
    emit("cmp x0, #0");
    emit("bge .Lprint_done");
    emit("sub x1, x1, #1");
    emit("mov w2, #'-'");
    emit("strb w2, [x1]");
    
    emitLabel(".Lprint_done");
    // calculate length
    emit("mov x2, sp");
    emit("add x2, x2, #31");
    emit("sub x2, x2, x1"); // length
    
    // write syscall
    emit("mov x0, #1"); // stdout
    // x1 is buffer
    // x2 is length
    emit("mov x8, #64");
    emit("svc #0");
    
    emit("mov sp, x29");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");
    m_out << "\n";

    for (const auto& fn : program->functions) {
        generateFnDecl(fn.get());
    }
}

void CodeGen::generateFnDecl(FnDecl* fn) {
    m_currentFunction = fn->name;
    m_out << ".global " << fn->name << "\n";
    emitLabel(fn->name);
    
    // Prologue
    emit("stp x29, x30, [sp, #-16]!");
    emit("mov x29, sp");
    
    m_currentFpOffset = 0;
    m_scopes.clear();
    m_variables.clear();
    
    enterScope();
    
    // Handle parameters
    // Parameters come in x0-x7. We push them to the stack to become local variables
    for (size_t i = 0; i < fn->params.size(); ++i) {
        emit("str x" + std::to_string(i) + ", [sp, #-16]!");
        declareVar(fn->params[i]);
    }
    
    // Body
    for (const auto& stmt : fn->body->statements) {
        generateStmt(stmt.get());
    }
    
    // Default return 0 if no return statement hit
    emit("mov x0, #0");
    emitLabel(".L" + m_currentFunction + "_end");
    
    // Epilogue
    emit("mov sp, x29");
    emit("ldp x29, x30, [sp], #16");
    emit("ret");
    m_out << "\n";
    
    leaveScope();
}

void CodeGen::generateBlock(Block* block) {
    enterScope();
    for (const auto& stmt : block->statements) {
        generateStmt(stmt.get());
    }
    leaveScope();
}

void CodeGen::generateStmt(Stmt* stmt) {
    if (auto assignStmt = dynamic_cast<AssignStmt*>(stmt)) {
        generateExpr(assignStmt->value.get());
        if (assignStmt->isDeclaration) {
            push("x0"); // Save value on stack as local variable
            declareVar(assignStmt->name);
        } else {
            int offset = getVarOffset(assignStmt->name);
            emit("str x0, [x29, #" + std::to_string(offset) + "]");
        }
    } else if (auto ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        generateExpr(ifStmt->condition.get());
        std::string elseLabel = newLabel();
        std::string endLabel = newLabel();
        
        emit("cmp x0, #0");
        if (ifStmt->elseBranch) {
            emit("beq " + elseLabel);
            generateStmt(ifStmt->thenBranch.get());
            emit("b " + endLabel);
            emitLabel(elseLabel);
            generateStmt(ifStmt->elseBranch.get());
            emitLabel(endLabel);
        } else {
            emit("beq " + endLabel);
            generateStmt(ifStmt->thenBranch.get());
            emitLabel(endLabel);
        }
    } else if (auto whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        std::string startLabel = newLabel();
        std::string endLabel = newLabel();
        
        emitLabel(startLabel);
        generateExpr(whileStmt->condition.get());
        emit("cmp x0, #0");
        emit("beq " + endLabel);
        generateStmt(whileStmt->body.get());
        emit("b " + startLabel);
        emitLabel(endLabel);
    } else if (auto retStmt = dynamic_cast<ReturnStmt*>(stmt)) {
        if (retStmt->value) {
            generateExpr(retStmt->value.get());
        } else {
            emit("mov x0, #0");
        }
        emit("b .L" + m_currentFunction + "_end");
    } else if (auto printStmt = dynamic_cast<PrintStmt*>(stmt)) {
        generateExpr(printStmt->value.get());
        emit("bl _print_int");
    } else if (auto exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        generateExpr(exprStmt->expr.get());
    } else if (auto blockStmt = dynamic_cast<Block*>(stmt)) {
        generateBlock(blockStmt);
    }
}

void CodeGen::generateExpr(Expr* expr) {
    if (auto intLit = dynamic_cast<IntLiteral*>(expr)) {
        // Load immediate, might need multiple instructions if large
        uint64_t val = (uint64_t)intLit->value;
        if (val <= 0xFFFF) {
            emit("mov x0, #" + std::to_string(val));
        } else {
            emit("movz x0, #" + std::to_string(val & 0xFFFF));
            if ((val >> 16) & 0xFFFF) emit("movk x0, #" + std::to_string((val >> 16) & 0xFFFF) + ", lsl #16");
            if ((val >> 32) & 0xFFFF) emit("movk x0, #" + std::to_string((val >> 32) & 0xFFFF) + ", lsl #32");
            if ((val >> 48) & 0xFFFF) emit("movk x0, #" + std::to_string((val >> 48) & 0xFFFF) + ", lsl #48");
        }
    } else if (auto boolLit = dynamic_cast<BoolLiteral*>(expr)) {
        emit("mov x0, #" + std::to_string(boolLit->value ? 1 : 0));
    } else if (auto id = dynamic_cast<Identifier*>(expr)) {
        int offset = getVarOffset(id->name);
        emit("ldr x0, [x29, #" + std::to_string(offset) + "]");
    } else if (auto binExpr = dynamic_cast<BinaryExpr*>(expr)) {
        if (binExpr->op == TokenType::AND) {
            generateExpr(binExpr->left.get());
            std::string falseLabel = newLabel();
            std::string endLabel = newLabel();
            emit("cmp x0, #0");
            emit("beq " + falseLabel);
            generateExpr(binExpr->right.get());
            emit("cmp x0, #0");
            emit("beq " + falseLabel);
            emit("mov x0, #1");
            emit("b " + endLabel);
            emitLabel(falseLabel);
            emit("mov x0, #0");
            emitLabel(endLabel);
            return;
        } else if (binExpr->op == TokenType::OR) {
            generateExpr(binExpr->left.get());
            std::string trueLabel = newLabel();
            std::string endLabel = newLabel();
            emit("cmp x0, #0");
            emit("bne " + trueLabel);
            generateExpr(binExpr->right.get());
            emit("cmp x0, #0");
            emit("bne " + trueLabel);
            emit("mov x0, #0");
            emit("b " + endLabel);
            emitLabel(trueLabel);
            emit("mov x0, #1");
            emitLabel(endLabel);
            return;
        }

        generateExpr(binExpr->left.get());
        push("x0"); // Save left side
        generateExpr(binExpr->right.get());
        emit("mov x1, x0"); // Right side to x1
        pop("x0"); // Restore left side to x0
        
        switch (binExpr->op) {
            case TokenType::PLUS: emit("add x0, x0, x1"); break;
            case TokenType::MINUS: emit("sub x0, x0, x1"); break;
            case TokenType::STAR: emit("mul x0, x0, x1"); break;
            case TokenType::SLASH: emit("sdiv x0, x0, x1"); break;
            case TokenType::PERCENT:
                emit("sdiv x2, x0, x1");
                emit("msub x0, x2, x1, x0");
                break;
            case TokenType::EQ:
            case TokenType::NEQ:
            case TokenType::LT:
            case TokenType::LTE:
            case TokenType::GT:
            case TokenType::GTE:
                emit("cmp x0, x1");
                if (binExpr->op == TokenType::EQ) emit("cset x0, eq");
                else if (binExpr->op == TokenType::NEQ) emit("cset x0, ne");
                else if (binExpr->op == TokenType::LT) emit("cset x0, lt");
                else if (binExpr->op == TokenType::LTE) emit("cset x0, le");
                else if (binExpr->op == TokenType::GT) emit("cset x0, gt");
                else if (binExpr->op == TokenType::GTE) emit("cset x0, ge");
                break;
            default: break;
        }
    } else if (auto unExpr = dynamic_cast<UnaryExpr*>(expr)) {
        generateExpr(unExpr->right.get());
        if (unExpr->op == TokenType::MINUS) {
            emit("neg x0, x0");
        } else if (unExpr->op == TokenType::NOT) {
            emit("cmp x0, #0");
            emit("cset x0, eq");
        }
    } else if (auto callExpr = dynamic_cast<CallExpr*>(expr)) {
        // Evaluate arguments and push them
        for (const auto& arg : callExpr->args) {
            generateExpr(arg.get());
            push("x0");
        }
        
        // Pop into registers x0-x7 (in reverse order)
        for (int i = (int)callExpr->args.size() - 1; i >= 0; --i) {
            pop("x" + std::to_string(i));
        }
        
        emit("bl " + callExpr->callee);
    }
}

} // namespace toy
