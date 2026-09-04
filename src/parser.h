#pragma once
#include <vector>
#include <memory>
#include "lexer.h"
#include "ast.h"
#include "error.h"

namespace toy {

class Parser {
public:
    Parser(const std::vector<Token>& tokens, ErrorReporter& reporter);
    std::unique_ptr<Program> parse();

private:
    std::unique_ptr<FnDecl> parseFnDecl();
    std::unique_ptr<Block> parseBlock();
    StmtPtr parseStatement();
    StmtPtr parseLetStmt();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parseReturnStmt();
    StmtPtr parsePrintStmt();
    StmtPtr parseExprStmtOrAssign();

    ExprPtr parseExpression();
    ExprPtr parseLogicOr();
    ExprPtr parseLogicAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parseUnary();
    ExprPtr parseCall();
    ExprPtr parsePrimary();

    bool match(TokenType type);
    bool check(TokenType type) const;
    Token advance();
    Token peek() const;
    Token previous() const;
    Token consume(TokenType type, std::string_view message);
    void synchronize();

    const std::vector<Token>& m_tokens;
    ErrorReporter& m_reporter;
    size_t m_current = 0;
};

} // namespace toy
