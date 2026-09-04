#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <cstdint>
#include "error.h"

namespace toy {

enum class TokenType {
    // Keywords
    DEF, IF, ELSE, WHILE, RETURN, PRINT, PRINT_STR, TRUE_LIT, FALSE_LIT,

    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQ, NEQ, LT, LTE, GT, GTE,
    AND, OR, NOT,

    // Punctuation
    LPAREN, RPAREN, LBRACKET, RBRACKET, COMMA, COLON,

    // Literals & Identifiers
    INTEGER, STRING, IDENTIFIER,

    // Special
    NEWLINE, INDENT, DEDENT, END_OF_FILE, ERROR
};

struct Token {
    TokenType type;
    std::string_view text;
    Location loc;
    int64_t int_val = 0; // For INTEGER
    std::string str_val; // For STRING
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
    std::vector<int> m_indentStack;
    bool m_isAtLineStart = true;
};

const char* tokenTypeToString(TokenType type);

} // namespace toy
