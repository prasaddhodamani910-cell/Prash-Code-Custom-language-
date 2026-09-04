#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <cstdint>
#include "error.h"

namespace toy {

enum class TokenType {
    // Keywords
    LET, FN, IF, ELSE, WHILE, RETURN, PRINT, TRUE_LIT, FALSE_LIT,

    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQ, NEQ, LT, LTE, GT, GTE,
    AND, OR, NOT,

    // Punctuation
    LPAREN, RPAREN, LBRACE, RBRACE,
    COMMA, SEMICOLON,

    // Literals & Identifiers
    INTEGER, IDENTIFIER,

    // Special
    END_OF_FILE, ERROR
};

struct Token {
    TokenType type;
    std::string_view text;
    Location loc;
    int64_t int_val = 0; // For INTEGER
};

class Lexer {
public:
    Lexer(std::string_view source, ErrorReporter& reporter);
    std::vector<Token> tokenize();

private:
    char advance();
    bool match(char expected);
    char peek() const;
    char peekNext() const;
    void skipWhitespace();
    Token makeToken(TokenType type);
    Token number();
    Token identifier();

    std::string_view m_source;
    ErrorReporter& m_reporter;
    size_t m_start = 0;
    size_t m_current = 0;
    int m_line = 1;
    int m_column = 1;
    int m_startColumn = 1;
};

const char* tokenTypeToString(TokenType type);

} // namespace toy
