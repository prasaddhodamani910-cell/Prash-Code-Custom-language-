import re

with open('src/codegen.cpp', 'r') as f:
    content = f.read()

# We want to replace from 'void CodeGen::generateStmt' to the end.
# So we slice it out.
idx = content.find('void CodeGen::generateStmt(Stmt* stmt)')
head = content[:idx]

new_code = """void CodeGen::generateStmt(Stmt* stmt) {
    if (auto assignStmt = dynamic_cast<AssignStmt*>(stmt)) {
        std::string reg = generateExpr(assignStmt->value.get());
        if (assignStmt->isDeclaration) {
            push(reg); // Save value on stack as local variable
            declareVar(assignStmt->name);
        } else {
            int offset = getVarOffset(assignStmt->name);
            emit("str " + reg + ", [x29, #" + std::to_string(offset) + "]");
        }
        freeReg(reg);
    } else if (auto ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        std::string reg = generateExpr(ifStmt->condition.get());
        std::string elseLabel = newLabel();
        std::string endLabel = newLabel();
        
        emit("cmp " + reg + ", #0");
        freeReg(reg);
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
        std::string reg = generateExpr(whileStmt->condition.get());
        emit("cmp " + reg + ", #0");
        freeReg(reg);
        emit("beq " + endLabel);
        generateStmt(whileStmt->body.get());
        emit("b " + startLabel);
        emitLabel(endLabel);
    } else if (auto retStmt = dynamic_cast<ReturnStmt*>(stmt)) {
        if (retStmt->value) {
            std::string reg = generateExpr(retStmt->value.get());
            emit("mov x0, " + reg);
            freeReg(reg);
        } else {
            emit("mov x0, #0");
        }
        emit("b .L" + m_currentFunction + "_end");
    } else if (auto printStmt = dynamic_cast<PrintStmt*>(stmt)) {
        std::string reg = generateExpr(printStmt->value.get());
        emit("mov x0, " + reg);
        freeReg(reg);
        emit("bl _print_int");
    } else if (auto exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        std::string reg = generateExpr(exprStmt->expr.get());
        freeReg(reg);
    } else if (auto blockStmt = dynamic_cast<Block*>(stmt)) {
        generateBlock(blockStmt);
    }
}

std::string CodeGen::generateExpr(Expr* expr) {
    if (auto intLit = dynamic_cast<IntLiteral*>(expr)) {
        std::string reg = allocReg();
        uint64_t val = (uint64_t)intLit->value;
        if (val <= 0xFFFF) {
            emit("mov " + reg + ", #" + std::to_string(val));
        } else {
            emit("movz " + reg + ", #" + std::to_string(val & 0xFFFF));
            if ((val >> 16) & 0xFFFF) emit("movk " + reg + ", #" + std::to_string((val >> 16) & 0xFFFF) + ", lsl #16");
            if ((val >> 32) & 0xFFFF) emit("movk " + reg + ", #" + std::to_string((val >> 32) & 0xFFFF) + ", lsl #32");
            if ((val >> 48) & 0xFFFF) emit("movk " + reg + ", #" + std::to_string((val >> 48) & 0xFFFF) + ", lsl #48");
        }
        return reg;
    } else if (auto boolLit = dynamic_cast<BoolLiteral*>(expr)) {
        std::string reg = allocReg();
        emit("mov " + reg + ", #" + std::to_string(boolLit->value ? 1 : 0));
        return reg;
    } else if (auto id = dynamic_cast<Identifier*>(expr)) {
        std::string reg = allocReg();
        int offset = getVarOffset(id->name);
        emit("ldr " + reg + ", [x29, #" + std::to_string(offset) + "]");
        return reg;
    } else if (auto binExpr = dynamic_cast<BinaryExpr*>(expr)) {
        if (binExpr->op == TokenType::AND) {
            std::string reg = generateExpr(binExpr->left.get());
            std::string falseLabel = newLabel();
            std::string endLabel = newLabel();
            emit("cmp " + reg + ", #0");
            emit("beq " + falseLabel);
            std::string rightReg = generateExpr(binExpr->right.get());
            emit("cmp " + rightReg + ", #0");
            emit("beq " + falseLabel);
            emit("mov " + reg + ", #1");
            emit("b " + endLabel);
            emitLabel(falseLabel);
            emit("mov " + reg + ", #0");
            emitLabel(endLabel);
            freeReg(rightReg);
            return reg;
        } else if (binExpr->op == TokenType::OR) {
            std::string reg = generateExpr(binExpr->left.get());
            std::string trueLabel = newLabel();
            std::string endLabel = newLabel();
            emit("cmp " + reg + ", #0");
            emit("bne " + trueLabel);
            std::string rightReg = generateExpr(binExpr->right.get());
            emit("cmp " + rightReg + ", #0");
            emit("bne " + trueLabel);
            emit("mov " + reg + ", #0");
            emit("b " + endLabel);
            emitLabel(trueLabel);
            emit("mov " + reg + ", #1");
            emitLabel(endLabel);
            freeReg(rightReg);
            return reg;
        }

        std::string leftReg = generateExpr(binExpr->left.get());
        std::string rightReg = generateExpr(binExpr->right.get());
        
        switch (binExpr->op) {
            case TokenType::PLUS: emit("add " + leftReg + ", " + leftReg + ", " + rightReg); break;
            case TokenType::MINUS: emit("sub " + leftReg + ", " + leftReg + ", " + rightReg); break;
            case TokenType::STAR: emit("mul " + leftReg + ", " + leftReg + ", " + rightReg); break;
            case TokenType::SLASH: emit("sdiv " + leftReg + ", " + leftReg + ", " + rightReg); break;
            case TokenType::PERCENT:
                emit("sdiv x2, " + leftReg + ", " + rightReg);
                emit("msub " + leftReg + ", x2, " + rightReg + ", " + leftReg);
                break;
            case TokenType::EQ:
            case TokenType::NEQ:
            case TokenType::LT:
            case TokenType::LTE:
            case TokenType::GT:
            case TokenType::GTE:
                emit("cmp " + leftReg + ", " + rightReg);
                if (binExpr->op == TokenType::EQ) emit("cset " + leftReg + ", eq");
                else if (binExpr->op == TokenType::NEQ) emit("cset " + leftReg + ", ne");
                else if (binExpr->op == TokenType::LT) emit("cset " + leftReg + ", lt");
                else if (binExpr->op == TokenType::LTE) emit("cset " + leftReg + ", le");
                else if (binExpr->op == TokenType::GT) emit("cset " + leftReg + ", gt");
                else if (binExpr->op == TokenType::GTE) emit("cset " + leftReg + ", ge");
                break;
            default: break;
        }
        freeReg(rightReg);
        return leftReg;
    } else if (auto unExpr = dynamic_cast<UnaryExpr*>(expr)) {
        std::string reg = generateExpr(unExpr->right.get());
        if (unExpr->op == TokenType::MINUS) {
            emit("neg " + reg + ", " + reg);
        } else if (unExpr->op == TokenType::NOT) {
            emit("cmp " + reg + ", #0");
            emit("cset " + reg + ", eq");
        }
        return reg;
    } else if (auto callExpr = dynamic_cast<CallExpr*>(expr)) {
        std::vector<std::string> argRegs;
        for (const auto& arg : callExpr->args) {
            argRegs.push_back(generateExpr(arg.get()));
        }
        
        for (size_t i = 0; i < callExpr->args.size(); ++i) {
            emit("mov x" + std::to_string(i) + ", " + argRegs[i]);
            freeReg(argRegs[i]);
        }
        
        emit("bl " + callExpr->callee);
        std::string resReg = allocReg();
        emit("mov " + resReg + ", x0");
        return resReg;
    }
    return allocReg();
}

} // namespace toy
"""

with open('src/codegen.cpp', 'w') as f:
    f.write(head + new_code)

