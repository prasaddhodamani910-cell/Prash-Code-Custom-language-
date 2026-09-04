#include "lexer.h"
#include <cctype>
#include <unordered_map>

namespace toy {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::LET: return "LET";
        case TokenType::FN: return "FN";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::WHILE: return "WHILE";
        case TokenType::RETURN: return "RETURN";
        case TokenType::PRINT: return "PRINT";
        case TokenType::TRUE_LIT: return "TRUE_LIT";
        case TokenType::FALSE_LIT: return "FALSE_LIT";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::LTE: return "LTE";
        case TokenType::GT: return "GT";
        case TokenType::GTE: return "GTE";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

Lexer::Lexer(std::string_view source, ErrorReporter& reporter)
    : m_source(source), m_reporter(reporter) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespace();
        m_start = m_current;
        m_startColumn = m_column;

        if (m_current >= m_source.length()) {
            tokens.push_back(makeToken(TokenType::END_OF_FILE));
            break;
        }

        char c = advance();

        if (std::isalpha(c) || c == '_') {
            tokens.push_back(identifier());
            continue;
        }
        if (std::isdigit(c)) {
            tokens.push_back(number());
            continue;
        }

        switch (c) {
            case '(': tokens.push_back(makeToken(TokenType::LPAREN)); break;
            case ')': tokens.push_back(makeToken(TokenType::RPAREN)); break;
            case '{': tokens.push_back(makeToken(TokenType::LBRACE)); break;
            case '}': tokens.push_back(makeToken(TokenType::RBRACE)); break;
            case ',': tokens.push_back(makeToken(TokenType::COMMA)); break;
            case ';': tokens.push_back(makeToken(TokenType::SEMICOLON)); break;
            case '+': tokens.push_back(makeToken(TokenType::PLUS)); break;
            case '-': tokens.push_back(makeToken(TokenType::MINUS)); break;
            case '*': tokens.push_back(makeToken(TokenType::STAR)); break;
            case '/': tokens.push_back(makeToken(TokenType::SLASH)); break;
            case '%': tokens.push_back(makeToken(TokenType::PERCENT)); break;
            case '=':
                tokens.push_back(makeToken(match('=') ? TokenType::EQ : TokenType::ASSIGN));
                break;
            case '!':
                tokens.push_back(makeToken(match('=') ? TokenType::NEQ : TokenType::NOT));
                break;
            case '<':
                tokens.push_back(makeToken(match('=') ? TokenType::LTE : TokenType::LT));
                break;
            case '>':
                tokens.push_back(makeToken(match('=') ? TokenType::GTE : TokenType::GT));
                break;
            case '&':
                if (match('&')) {
                    tokens.push_back(makeToken(TokenType::AND));
                } else {
                    m_reporter.error({m_line, m_column - 1}, "Expected '&' after '&'");
                    tokens.push_back(makeToken(TokenType::ERROR));
                }
                break;
            case '|':
                if (match('|')) {
                    tokens.push_back(makeToken(TokenType::OR));
                } else {
                    m_reporter.error({m_line, m_column - 1}, "Expected '|' after '|'");
                    tokens.push_back(makeToken(TokenType::ERROR));
                }
                break;
            default:
                m_reporter.error({m_line, m_column - 1}, std::string("Unexpected character: ") + c);
                tokens.push_back(makeToken(TokenType::ERROR));
                break;
        }
    }
    return tokens;
}

char Lexer::advance() {
    m_column++;
    return m_source[m_current++];
}

bool Lexer::match(char expected) {
    if (m_current >= m_source.length()) return false;
    if (m_source[m_current] != expected) return false;
    m_current++;
    m_column++;
    return true;
}

char Lexer::peek() const {
    if (m_current >= m_source.length()) return '\0';
    return m_source[m_current];
}

char Lexer::peekNext() const {
    if (m_current + 1 >= m_source.length()) return '\0';
    return m_source[m_current + 1];
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                m_line++;
                m_column = 0; // Will become 1 after advance() increments it? Wait.
                // Actually, if we hit \n, advance() increments m_column. So we just reset it here.
                m_current++;
                m_column = 1;
                break;
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && peek() != '\0') {
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token Lexer::makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.text = m_source.substr(m_start, m_current - m_start);
    token.loc = {m_line, m_startColumn};
    return token;
}

Token Lexer::number() {
    while (std::isdigit(peek())) {
        advance();
    }
    Token t = makeToken(TokenType::INTEGER);
    std::string text(t.text);
    t.int_val = std::stoll(text);
    return t;
}

Token Lexer::identifier() {
    while (std::isalnum(peek()) || peek() == '_') {
        advance();
    }
    std::string_view text = m_source.substr(m_start, m_current - m_start);
    TokenType type = TokenType::IDENTIFIER;
    
    if (text == "let") type = TokenType::LET;
    else if (text == "fn") type = TokenType::FN;
    else if (text == "if") type = TokenType::IF;
    else if (text == "else") type = TokenType::ELSE;
    else if (text == "while") type = TokenType::WHILE;
    else if (text == "return") type = TokenType::RETURN;
    else if (text == "print") type = TokenType::PRINT;
    else if (text == "true") type = TokenType::TRUE_LIT;
    else if (text == "false") type = TokenType::FALSE_LIT;

    return makeToken(type);
}

} // namespace toy
