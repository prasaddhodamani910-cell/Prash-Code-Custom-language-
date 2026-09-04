#include "parser.h"
#include <stdexcept>

namespace toy {

Parser::Parser(std::vector<Token> tokens, ErrorReporter& reporter)
    : m_tokens(std::move(tokens)), m_reporter(reporter) {}

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();
    while (!check(TokenType::END_OF_FILE)) {
        if (match(TokenType::NEWLINE)) continue; // ignore stray newlines
        try {
            auto fn = parseFnDecl();
            if (fn) {
                program->functions.push_back(std::move(fn));
            }
        } catch (const std::exception& e) {
            fprintf(stderr, "Caught exception: %s\n", e.what());
            synchronize();
        }
    }
    return program;
}

std::unique_ptr<FnDecl> Parser::parseFnDecl() {
    Location loc = peek().loc;
    consume(TokenType::DEF, "Expected 'def' keyword");
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    
    consume(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::string> params;
    if (!check(TokenType::RPAREN)) {
        do {
            if (params.size() >= 8) {
                m_reporter.error(peek().loc, "Cannot have more than 8 parameters");
            }
            params.push_back(std::string(consume(TokenType::IDENTIFIER, "Expected parameter name").text));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    consume(TokenType::COLON, "Expected ':' after function signature");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    
    std::unique_ptr<Block> body = parseBlock();
    
    return std::make_unique<FnDecl>(std::string(name.text), std::move(params), std::move(body), loc);
}

std::unique_ptr<Block> Parser::parseBlock() {
    Location loc = peek().loc;
    consume(TokenType::INDENT, "Expected indentation");
    std::vector<StmtPtr> statements;
    while (!check(TokenType::DEDENT) && !check(TokenType::END_OF_FILE)) {
        if (match(TokenType::NEWLINE)) continue;
        statements.push_back(parseStatement());
    }
    consume(TokenType::DEDENT, "Expected dedent");
    return std::make_unique<Block>(std::move(statements), loc);
}

StmtPtr Parser::parseStatement() {
    if (match(TokenType::IF)) return parseIfStmt();
    if (match(TokenType::WHILE)) return parseWhileStmt();
    if (match(TokenType::RETURN)) return parseReturnStmt();
    if (match(TokenType::PRINT)) return parsePrintStmt();
    if (match(TokenType::PRINT_STR)) return parsePrintStrStmt();
    
    return parseExprStmtOrAssign();
}

StmtPtr Parser::parseIfStmt() {
    Location loc = previous().loc;
    ExprPtr condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after if condition");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    
    StmtPtr thenBranch = parseBlock();
    StmtPtr elseBranch = nullptr;
    
    if (match(TokenType::ELSE)) {
        consume(TokenType::COLON, "Expected ':' after else");
        consume(TokenType::NEWLINE, "Expected newline after ':'");
        elseBranch = parseBlock();
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), loc);
}

StmtPtr Parser::parseWhileStmt() {
    Location loc = previous().loc;
    ExprPtr condition = parseExpression();
    consume(TokenType::COLON, "Expected ':' after while condition");
    consume(TokenType::NEWLINE, "Expected newline after ':'");
    
    StmtPtr body = parseBlock();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), loc);
}

StmtPtr Parser::parseReturnStmt() {
    Location loc = previous().loc;
    ExprPtr value = nullptr;
    if (!check(TokenType::NEWLINE) && !check(TokenType::END_OF_FILE)) {
        value = parseExpression();
    }
    consume(TokenType::NEWLINE, "Expected newline after return value");
    return std::make_unique<ReturnStmt>(std::move(value), loc);
}

StmtPtr Parser::parsePrintStmt() {
    Location loc = previous().loc;
    consume(TokenType::LPAREN, "Expected '(' after 'print'");
    ExprPtr value = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after print argument");
    consume(TokenType::NEWLINE, "Expected newline after print statement");
    return std::make_unique<PrintStmt>(std::move(value), loc);
}

StmtPtr Parser::parsePrintStrStmt() {
    Location loc = previous().loc;
    consume(TokenType::LPAREN, "Expected '(' after 'print_str'");
    ExprPtr value = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after print_str argument");
    consume(TokenType::NEWLINE, "Expected newline after print_str statement");
    return std::make_unique<PrintStrStmt>(std::move(value), loc);
}

StmtPtr Parser::parseExprStmtOrAssign() {
    Location loc = peek().loc;
    ExprPtr expr = parseExpression();
    
    if (match(TokenType::ASSIGN)) {
        if (auto id = dynamic_cast<Identifier*>(expr.get())) {
            ExprPtr value = parseExpression();
            consume(TokenType::NEWLINE, "Expected newline after assignment");
            return std::make_unique<AssignStmt>(id->name, std::move(value), loc);
        } else if (auto idx = dynamic_cast<IndexExpr*>(expr.get())) {
            ExprPtr value = parseExpression();
            consume(TokenType::NEWLINE, "Expected newline after assignment");
            return std::make_unique<IndexAssignStmt>(std::move(idx->array), std::move(idx->index), std::move(value), loc);
        } else {
            m_reporter.error(loc, "Invalid assignment target");
        }
    }
    
    if (!check(TokenType::END_OF_FILE) && !check(TokenType::DEDENT)) {
        consume(TokenType::NEWLINE, "Expected newline after expression");
    }
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
            expr = std::make_unique<CallExpr>(callee, std::move(args), loc);
        } else {
            m_reporter.error(loc, "Can only call functions");
            while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) advance();
            if (check(TokenType::RPAREN)) advance();
        }
    }
    
    while (match(TokenType::LBRACKET)) {
        Location loc = previous().loc;
        ExprPtr index = parseExpression();
        consume(TokenType::RBRACKET, "Expected ']' after index");
        expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index), loc);
    }
    
    return expr;
}

ExprPtr Parser::parsePrimary() {
    if (match(TokenType::FALSE_LIT)) return std::make_unique<BoolLiteral>(false, previous().loc);
    if (match(TokenType::TRUE_LIT)) return std::make_unique<BoolLiteral>(true, previous().loc);
    
    if (match(TokenType::INTEGER)) {
        return std::make_unique<IntLiteral>(previous().int_val, previous().loc);
    }
    
    if (match(TokenType::STRING)) {
        return std::make_unique<StringLiteral>(previous().str_val, previous().loc);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<Identifier>(std::string(previous().text), previous().loc);
    }
    
    if (match(TokenType::LPAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    if (match(TokenType::LBRACKET)) {
        Location loc = previous().loc;
        std::vector<ExprPtr> elements;
        if (!check(TokenType::RBRACKET)) {
            do {
                elements.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RBRACKET, "Expected ']' after array elements");
        return std::make_unique<ArrayLiteral>(std::move(elements), loc);
    }
    
    m_reporter.error(peek().loc, "Expected expression");
    throw std::runtime_error("Parse error");
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (m_current >= m_tokens.size()) return type == TokenType::END_OF_FILE;
    return peek().type == type;
}

Token Parser::advance() {
    if (!check(TokenType::END_OF_FILE) && m_current < m_tokens.size()) m_current++;
    return previous();
}

Token Parser::peek() const {
    if (m_current >= m_tokens.size()) {
        if (!m_tokens.empty()) return m_tokens.back();
        return Token{TokenType::END_OF_FILE, "", {0, 0}, 0, ""};
    }
    return m_tokens[m_current];
}

Token Parser::previous() const {
    if (m_current == 0) return peek();
    if (m_current - 1 >= m_tokens.size()) return m_tokens.back();
    return m_tokens[m_current - 1];
}

Token Parser::consume(TokenType type, std::string_view message) {
    if (check(type)) return advance();
    m_reporter.error(peek().loc, message);
    throw std::runtime_error("Parse error");
}

void Parser::synchronize() {
    advance();
    while (!check(TokenType::END_OF_FILE)) {
        if (previous().type == TokenType::NEWLINE) return;
        if (check(TokenType::DEDENT)) return;
        switch (peek().type) {
            case TokenType::DEF:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
            case TokenType::PRINT:
            case TokenType::PRINT_STR:
                return;
            default:
                break;
        }
        advance();
    }
}

} // namespace toy
