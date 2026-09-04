#include "parser.h"

namespace toy {

Parser::Parser(const std::vector<Token>& tokens, ErrorReporter& reporter)
    : m_tokens(tokens), m_reporter(reporter) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    while (!check(TokenType::END_OF_FILE)) {
        if (match(TokenType::FN)) {
            program->functions.push_back(parseFnDecl());
        } else {
            m_reporter.error(peek().loc, "Expected 'fn' declaration");
            synchronize();
        }
    }
    return program;
}

std::unique_ptr<FnDecl> Parser::parseFnDecl() {
    Token nameToken = consume(TokenType::IDENTIFIER, "Expected function name");
    consume(TokenType::LPAREN, "Expected '(' after function name");
    
    std::vector<std::string> params;
    if (!check(TokenType::RPAREN)) {
        do {
            params.push_back(std::string(consume(TokenType::IDENTIFIER, "Expected parameter name").text));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    
    consume(TokenType::LBRACE, "Expected '{' before function body");
    auto body = parseBlock();
    
    return std::make_unique<FnDecl>(std::string(nameToken.text), std::move(params), std::move(body), nameToken.loc);
}

std::unique_ptr<Block> Parser::parseBlock() {
    std::vector<StmtPtr> statements;
    Location loc = previous().loc;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        statements.push_back(parseStatement());
    }
    consume(TokenType::RBRACE, "Expected '}' after block");
    return std::make_unique<Block>(std::move(statements), loc);
}

StmtPtr Parser::parseStatement() {
    if (match(TokenType::LET)) return parseLetStmt();
    if (match(TokenType::IF)) return parseIfStmt();
    if (match(TokenType::WHILE)) return parseWhileStmt();
    if (match(TokenType::RETURN)) return parseReturnStmt();
    if (match(TokenType::PRINT)) return parsePrintStmt();
    if (match(TokenType::LBRACE)) return parseBlock();
    
    return parseExprStmtOrAssign();
}

StmtPtr Parser::parseLetStmt() {
    Location loc = previous().loc;
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    consume(TokenType::ASSIGN, "Expected '=' after variable name");
    ExprPtr initializer = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return std::make_unique<LetStmt>(std::string(name.text), std::move(initializer), loc);
}

StmtPtr Parser::parseIfStmt() {
    Location loc = previous().loc;
    consume(TokenType::LPAREN, "Expected '(' after 'if'");
    ExprPtr condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after if condition");
    
    StmtPtr thenBranch = parseStatement();
    StmtPtr elseBranch = nullptr;
    
    if (match(TokenType::ELSE)) {
        elseBranch = parseStatement();
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), loc);
}

StmtPtr Parser::parseWhileStmt() {
    Location loc = previous().loc;
    consume(TokenType::LPAREN, "Expected '(' after 'while'");
    ExprPtr condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after while condition");
    
    StmtPtr body = parseStatement();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), loc);
}

StmtPtr Parser::parseReturnStmt() {
    Location loc = previous().loc;
    ExprPtr value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = parseExpression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after return value");
    return std::make_unique<ReturnStmt>(std::move(value), loc);
}

StmtPtr Parser::parsePrintStmt() {
    Location loc = previous().loc;
    consume(TokenType::LPAREN, "Expected '(' after 'print'");
    ExprPtr value = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after print argument");
    consume(TokenType::SEMICOLON, "Expected ';' after print statement");
    return std::make_unique<PrintStmt>(std::move(value), loc);
}

StmtPtr Parser::parseExprStmtOrAssign() {
    Location loc = peek().loc;
    ExprPtr expr = parseExpression();
    
    if (match(TokenType::ASSIGN)) {
        if (auto id = dynamic_cast<Identifier*>(expr.get())) {
            ExprPtr value = parseExpression();
            consume(TokenType::SEMICOLON, "Expected ';' after assignment");
            return std::make_unique<AssignStmt>(id->name, std::move(value), loc);
        } else {
            m_reporter.error(loc, "Invalid assignment target");
        }
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after expression");
    return std::make_unique<ExprStmt>(std::move(expr), loc);
}

ExprPtr Parser::parseExpression() {
    return parseLogicOr();
}

ExprPtr Parser::parseLogicOr() {
    ExprPtr expr = parseLogicAnd();
    while (match(TokenType::OR)) {
        Token op = previous();
        ExprPtr right = parseLogicAnd();
        expr = std::make_unique<BinaryExpr>(op.type, std::move(expr), std::move(right), op.loc);
    }
    return expr;
}

ExprPtr Parser::parseLogicAnd() {
    ExprPtr expr = parseEquality();
    while (match(TokenType::AND)) {
        Token op = previous();
        ExprPtr right = parseEquality();
        expr = std::make_unique<BinaryExpr>(op.type, std::move(expr), std::move(right), op.loc);
    }
    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseComparison();
    while (match(TokenType::EQ) || match(TokenType::NEQ)) {
        Token op = previous();
        ExprPtr right = parseComparison();
        expr = std::make_unique<BinaryExpr>(op.type, std::move(expr), std::move(right), op.loc);
    }
    return expr;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expr = parseTerm();
    while (match(TokenType::GT) || match(TokenType::GTE) || match(TokenType::LT) || match(TokenType::LTE)) {
        Token op = previous();
        ExprPtr right = parseTerm();
        expr = std::make_unique<BinaryExpr>(op.type, std::move(expr), std::move(right), op.loc);
    }
    return expr;
}

ExprPtr Parser::parseTerm() {
    ExprPtr expr = parseFactor();
    while (match(TokenType::MINUS) || match(TokenType::PLUS)) {
        Token op = previous();
        ExprPtr right = parseFactor();
        expr = std::make_unique<BinaryExpr>(op.type, std::move(expr), std::move(right), op.loc);
    }
    return expr;
}

ExprPtr Parser::parseFactor() {
    ExprPtr expr = parseUnary();
    while (match(TokenType::SLASH) || match(TokenType::STAR) || match(TokenType::PERCENT)) {
        Token op = previous();
        ExprPtr right = parseUnary();
        expr = std::make_unique<BinaryExpr>(op.type, std::move(expr), std::move(right), op.loc);
    }
    return expr;
}

ExprPtr Parser::parseUnary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS)) {
        Token op = previous();
        ExprPtr right = parseUnary();
        return std::make_unique<UnaryExpr>(op.type, std::move(right), op.loc);
    }
    return parseCall();
}

ExprPtr Parser::parseCall() {
    ExprPtr expr = parsePrimary();
    
    if (match(TokenType::LPAREN)) {
        Location loc = previous().loc;
        if (auto id = dynamic_cast<Identifier*>(expr.get())) {
            std::string callee = id->name;
            std::vector<ExprPtr> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "Expected ')' after arguments");
            return std::make_unique<CallExpr>(callee, std::move(args), loc);
        } else {
            m_reporter.error(loc, "Can only call functions");
            // Recover by consuming up to ')'
            while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) advance();
            if (check(TokenType::RPAREN)) advance();
            return expr;
        }
    }
    
    return expr;
}

ExprPtr Parser::parsePrimary() {
    if (match(TokenType::FALSE_LIT)) return std::make_unique<BoolLiteral>(false, previous().loc);
    if (match(TokenType::TRUE_LIT)) return std::make_unique<BoolLiteral>(true, previous().loc);
    
    if (match(TokenType::INTEGER)) {
        return std::make_unique<IntLiteral>(previous().int_val, previous().loc);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<Identifier>(std::string(previous().text), previous().loc);
    }
    
    if (match(TokenType::LPAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    m_reporter.error(peek().loc, "Expected expression");
    advance(); // Consume the invalid token
    return std::make_unique<IntLiteral>(0, previous().loc); // Dummy to avoid nulls
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (m_current >= m_tokens.size()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!check(TokenType::END_OF_FILE)) m_current++;
    return previous();
}

Token Parser::peek() const {
    return m_tokens[m_current];
}

Token Parser::previous() const {
    return m_tokens[m_current - 1];
}

Token Parser::consume(TokenType type, std::string_view message) {
    if (check(type)) return advance();
    m_reporter.error(peek().loc, message);
    return peek();
}

void Parser::synchronize() {
    advance();
    while (!check(TokenType::END_OF_FILE)) {
        if (previous().type == TokenType::SEMICOLON) return;
        switch (peek().type) {
            case TokenType::FN:
            case TokenType::LET:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
            case TokenType::PRINT:
                return;
            default:
                break;
        }
        advance();
    }
}

} // namespace toy
