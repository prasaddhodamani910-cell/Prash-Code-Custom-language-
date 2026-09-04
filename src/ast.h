#pragma once
#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <iostream>
#include "lexer.h"

namespace toy {

// Forward declarations
struct Program;
struct FnDecl;
struct Block;
struct Stmt;
struct Expr;

// Expressions
struct BinaryExpr;
struct UnaryExpr;
struct CallExpr;
struct IntLiteral;
struct BoolLiteral;
struct Identifier;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct Expr {
    virtual ~Expr() = default;
    virtual void dump(int indent) const = 0;
    Location loc;
};

struct IntLiteral : public Expr {
    int64_t value;
    IntLiteral(int64_t v, Location l) : value(v) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "IntLiteral(" << value << ")\n";
    }
};

struct BoolLiteral : public Expr {
    bool value;
    BoolLiteral(bool v, Location l) : value(v) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "BoolLiteral(" << (value ? "true" : "false") << ")\n";
    }
};

struct Identifier : public Expr {
    std::string name;
    Identifier(std::string n, Location l) : name(std::move(n)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "Identifier(" << name << ")\n";
    }
};

struct BinaryExpr : public Expr {
    TokenType op;
    ExprPtr left;
    ExprPtr right;
    BinaryExpr(TokenType op, ExprPtr l, ExprPtr r, Location loc) 
        : op(op), left(std::move(l)), right(std::move(r)) { this->loc = loc; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "BinaryExpr(" << tokenTypeToString(op) << ")\n";
        left->dump(indent + 2);
        right->dump(indent + 2);
    }
};

struct UnaryExpr : public Expr {
    TokenType op;
    ExprPtr right;
    UnaryExpr(TokenType op, ExprPtr r, Location loc) 
        : op(op), right(std::move(r)) { this->loc = loc; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "UnaryExpr(" << tokenTypeToString(op) << ")\n";
        right->dump(indent + 2);
    }
};

struct CallExpr : public Expr {
    std::string callee;
    std::vector<ExprPtr> args;
    CallExpr(std::string c, std::vector<ExprPtr> a, Location l) 
        : callee(std::move(c)), args(std::move(a)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "CallExpr(" << callee << ")\n";
        for (const auto& arg : args) {
            arg->dump(indent + 2);
        }
    }
};

// Statements
struct Stmt {
    virtual ~Stmt() = default;
    virtual void dump(int indent) const = 0;
    Location loc;
};

struct AssignStmt : public Stmt {
    std::string name;
    ExprPtr value;
    bool isDeclaration = false;
    AssignStmt(std::string n, ExprPtr v, Location l) 
        : name(std::move(n)), value(std::move(v)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "AssignStmt(" << name << ")\n";
        value->dump(indent + 2);
    }
};

struct Block : public Stmt {
    std::vector<StmtPtr> statements;
    Block(std::vector<StmtPtr> stmts, Location l) : statements(std::move(stmts)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "Block\n";
        for (const auto& stmt : statements) {
            stmt->dump(indent + 2);
        }
    }
};

struct IfStmt : public Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch; // can be null
    IfStmt(ExprPtr cond, StmtPtr th, StmtPtr el, Location l) 
        : condition(std::move(cond)), thenBranch(std::move(th)), elseBranch(std::move(el)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "IfStmt\n";
        condition->dump(indent + 2);
        thenBranch->dump(indent + 2);
        if (elseBranch) elseBranch->dump(indent + 2);
    }
};

struct WhileStmt : public Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr cond, StmtPtr b, Location l) 
        : condition(std::move(cond)), body(std::move(b)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "WhileStmt\n";
        condition->dump(indent + 2);
        body->dump(indent + 2);
    }
};

struct ReturnStmt : public Stmt {
    ExprPtr value;
    ReturnStmt(ExprPtr v, Location l) : value(std::move(v)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "ReturnStmt\n";
        value->dump(indent + 2);
    }
};

struct PrintStmt : public Stmt {
    ExprPtr value;
    PrintStmt(ExprPtr v, Location l) : value(std::move(v)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "PrintStmt\n";
        value->dump(indent + 2);
    }
};

struct ExprStmt : public Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr e, Location l) : expr(std::move(e)) { loc = l; }
    void dump(int indent) const override {
        std::cout << std::string(indent, ' ') << "ExprStmt\n";
        expr->dump(indent + 2);
    }
};

struct FnDecl {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<Block> body;
    Location loc;

    FnDecl(std::string n, std::vector<std::string> p, std::unique_ptr<Block> b, Location l)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)), loc(l) {}

    void dump(int indent) const {
        std::cout << std::string(indent, ' ') << "FnDecl(" << name << ")\n";
        for (const auto& p : params) {
            std::cout << std::string(indent + 2, ' ') << "Param(" << p << ")\n";
        }
        body->dump(indent + 2);
    }
};

struct Program {
    std::vector<std::unique_ptr<FnDecl>> functions;
    
    void dump(int indent = 0) const {
        std::cout << std::string(indent, ' ') << "Program\n";
        for (const auto& fn : functions) {
            fn->dump(indent + 2);
        }
    }
};

} // namespace toy
