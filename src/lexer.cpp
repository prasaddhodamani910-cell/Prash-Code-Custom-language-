#include "lexer.h"
#include <cctype>
#include <unordered_map>

namespace toy {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::DEF: return "DEF";
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
        case TokenType::COMMA: return "COMMA";
        case TokenType::COLON: return "COLON";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::INDENT: return "INDENT";
        case TokenType::DEDENT: return "DEDENT";
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

Lexer::Lexer(std::string_view source, ErrorReporter& reporter)
    : m_source(source), m_reporter(reporter) {
    m_indentStack.push_back(0);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        if (m_isAtLineStart) {
            int spaces = 0;
            while (peek() == ' ' || peek() == '\t') {
                if (peek() == '\t') spaces += 4;
                else spaces += 1;
                advance();
            }

            if (peek() == '\n' || peek() == '\r' || peek() == '#') {
                // Ignore empty lines or lines with only comments.
                // We let skipWhitespace skip the comment, but not the newline!
            } else if (peek() == '\0') {
                // EOF on a line start, handled later
            } else {
                m_isAtLineStart = false;

                if (spaces > m_indentStack.back()) {
                    m_indentStack.push_back(spaces);
                    Token t;
                    t.type = TokenType::INDENT;
                    t.text = "";
                    t.loc = {m_line, m_column};
                    tokens.push_back(t);
                } else if (spaces < m_indentStack.back()) {
                    while (spaces < m_indentStack.back()) {
                        m_indentStack.pop_back();
                        Token t;
                        t.type = TokenType::DEDENT;
                        t.text = "";
                        t.loc = {m_line, m_column};
                        tokens.push_back(t);
                    }
                    if (spaces != m_indentStack.back()) {
                        m_reporter.error({m_line, m_column}, "Indentation error");
                    }
                }
            }
        }

        skipWhitespace();
        m_start = m_current;
        m_startColumn = m_column;

        if (m_current >= m_source.length()) {
            if (tokens.size() > 0 && 
                tokens.back().type != TokenType::NEWLINE && 
                tokens.back().type != TokenType::INDENT && 
                tokens.back().type != TokenType::DEDENT) {
                Token t;
                t.type = TokenType::NEWLINE;
                t.text = "";
                t.loc = {m_line, m_column};
                tokens.push_back(t);
            }
            while (m_indentStack.size() > 1) {
                m_indentStack.pop_back();
                Token t;
                t.type = TokenType::DEDENT;
                t.text = "";
                t.loc = {m_line, m_column};
                tokens.push_back(t);
            }
            tokens.push_back(makeToken(TokenType::END_OF_FILE));
            break;
        }

        char c = advance();

        if (c == '\n') {
            m_isAtLineStart = true;
            m_line++;
            m_column = 1;
            
            if (!tokens.empty() && 
                tokens.back().type != TokenType::NEWLINE && 
                tokens.back().type != TokenType::INDENT && 
                tokens.back().type != TokenType::DEDENT) {
                Token t;
                t.type = TokenType::NEWLINE;
                t.text = "";
                t.loc = {m_line - 1, m_startColumn};
                tokens.push_back(t);
            }
            continue;
        }
        
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
            case ',': tokens.push_back(makeToken(TokenType::COMMA)); break;
            case ':': tokens.push_back(makeToken(TokenType::COLON)); break;
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
            case '\r':
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
            case '\t':
                advance();
                break;
            case '#':
                while (peek() != '\n' && peek() != '\0') {
                    advance();
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
    
    if (text == "def") type = TokenType::DEF;
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
