#include "browser/javascript/lexer.hpp"
#include <cctype>
#include <charconv>

namespace browser::javascript {

Lexer::Lexer(std::string_view source)
    : m_source(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        skipWhitespaceAndComments();
        if (isAtEnd()) break;
        m_start = m_current;
        tokens.push_back(scanToken());
    }
    tokens.push_back(Token{TokenType::EndOfFile, "", 0.0, m_line, m_column});
    return tokens;
}

bool Lexer::isAtEnd() const noexcept {
    return m_current >= m_source.size();
}

char Lexer::advance() {
    char c = m_source[m_current++];
    m_column++;
    return c;
}

char Lexer::peek() const noexcept {
    if (isAtEnd()) return '\0';
    return m_source[m_current];
}

char Lexer::peekNext() const noexcept {
    if (m_current + 1 >= m_source.size()) return '\0';
    return m_source[m_current + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (m_source[m_current] != expected) return false;
    m_current++;
    m_column++;
    return true;
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
        } else if (c == '\n') {
            advance();
            m_line++;
            m_column = 1;
        } else if (c == '/' && peekNext() == '/') {
            // Line comment
            while (!isAtEnd() && peek() != '\n') advance();
        } else if (c == '/' && peekNext() == '*') {
            // Block comment
            advance(); // '/'
            advance(); // '*'
            while (!isAtEnd() && !(peek() == '*' && peekNext() == '/')) {
                if (peek() == '\n') {
                    m_line++;
                    m_column = 1;
                }
                advance();
            }
            if (!isAtEnd()) advance(); // '*'
            if (!isAtEnd()) advance(); // '/'
        } else {
            break;
        }
    }
}

Token Lexer::scanToken() {
    size_t tokenLine = m_line;
    size_t tokenCol = m_column;
    char c = advance();

    switch (c) {
        case '(': return Token{TokenType::LeftParen, "(", 0.0, tokenLine, tokenCol};
        case ')': return Token{TokenType::RightParen, ")", 0.0, tokenLine, tokenCol};
        case '{': return Token{TokenType::LeftBrace, "{", 0.0, tokenLine, tokenCol};
        case '}': return Token{TokenType::RightBrace, "}", 0.0, tokenLine, tokenCol};
        case '[': return Token{TokenType::LeftBracket, "[", 0.0, tokenLine, tokenCol};
        case ']': return Token{TokenType::RightBracket, "]", 0.0, tokenLine, tokenCol};
        case ';': return Token{TokenType::Semicolon, ";", 0.0, tokenLine, tokenCol};
        case ',': return Token{TokenType::Comma, ",", 0.0, tokenLine, tokenCol};
        case '.': return Token{TokenType::Dot, ".", 0.0, tokenLine, tokenCol};

        case '+':
            if (match('+')) return Token{TokenType::PlusPlus, "++", 0.0, tokenLine, tokenCol};
            return Token{TokenType::Plus, "+", 0.0, tokenLine, tokenCol};
        case '-':
            if (match('-')) return Token{TokenType::MinusMinus, "--", 0.0, tokenLine, tokenCol};
            return Token{TokenType::Minus, "-", 0.0, tokenLine, tokenCol};
        case '*': return Token{TokenType::Star, "*", 0.0, tokenLine, tokenCol};
        case '/': return Token{TokenType::Slash, "/", 0.0, tokenLine, tokenCol};
        case '%': return Token{TokenType::Percent, "%", 0.0, tokenLine, tokenCol};

        case '=':
            if (match('=')) return Token{TokenType::EqualEqual, "==", 0.0, tokenLine, tokenCol};
            return Token{TokenType::Equal, "=", 0.0, tokenLine, tokenCol};
        case '!':
            if (match('=')) return Token{TokenType::BangEqual, "!=", 0.0, tokenLine, tokenCol};
            return Token{TokenType::Bang, "!", 0.0, tokenLine, tokenCol};
        case '<':
            if (match('=')) return Token{TokenType::LessEqual, "<=", 0.0, tokenLine, tokenCol};
            return Token{TokenType::Less, "<", 0.0, tokenLine, tokenCol};
        case '>':
            if (match('=')) return Token{TokenType::GreaterEqual, ">=", 0.0, tokenLine, tokenCol};
            return Token{TokenType::Greater, ">", 0.0, tokenLine, tokenCol};

        case '&':
            if (match('&')) return Token{TokenType::AmpAmp, "&&", 0.0, tokenLine, tokenCol};
            break;
        case '|':
            if (match('|')) return Token{TokenType::PipePipe, "||", 0.0, tokenLine, tokenCol};
            break;

        case '"':
        case '\'':
            return scanString(c);

        default:
            if (std::isdigit(static_cast<unsigned char>(c))) {
                m_current--;
                m_column--;
                return scanNumber();
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
                m_current--;
                m_column--;
                return scanIdentifierOrKeyword();
            }
            break;
    }

    return Token{TokenType::EndOfFile, std::string(1, c), 0.0, tokenLine, tokenCol};
}

Token Lexer::scanString(char quote) {
    size_t tokenLine = m_line;
    size_t tokenCol = m_column;
    std::string val;

    while (!isAtEnd() && peek() != quote) {
        char c = advance();
        if (c == '\\' && !isAtEnd()) {
            char esc = advance();
            switch (esc) {
                case 'n': val.push_back('\n'); break;
                case 't': val.push_back('\t'); break;
                case 'r': val.push_back('\r'); break;
                case '\\': val.push_back('\\'); break;
                case '\'': val.push_back('\''); break;
                case '"': val.push_back('"'); break;
                default: val.push_back(esc); break;
            }
        } else {
            if (c == '\n') {
                m_line++;
                m_column = 1;
            }
            val.push_back(c);
        }
    }

    if (!isAtEnd()) advance(); // consume closing quote
    return Token{TokenType::String, std::move(val), 0.0, tokenLine, tokenCol};
}

Token Lexer::scanNumber() {
    size_t tokenLine = m_line;
    size_t tokenCol = m_column;
    size_t start = m_current;

    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance(); // consume '.'
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    std::string text = std::string(m_source.substr(start, m_current - start));
    double num = 0.0;
    try {
        num = std::stod(text);
    } catch (...) {}

    return Token{TokenType::Number, std::move(text), num, tokenLine, tokenCol};
}

Token Lexer::scanIdentifierOrKeyword() {
    size_t tokenLine = m_line;
    size_t tokenCol = m_column;
    size_t start = m_current;

    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
            advance();
        } else {
            break;
        }
    }

    std::string text = std::string(m_source.substr(start, m_current - start));

    TokenType type = TokenType::Identifier;
    if (text == "var") type = TokenType::Var;
    else if (text == "let") type = TokenType::Let;
    else if (text == "const") type = TokenType::Const;
    else if (text == "function") type = TokenType::Function;
    else if (text == "return") type = TokenType::Return;
    else if (text == "if") type = TokenType::If;
    else if (text == "else") type = TokenType::Else;
    else if (text == "while") type = TokenType::While;
    else if (text == "for") type = TokenType::For;
    else if (text == "true") type = TokenType::True;
    else if (text == "false") type = TokenType::False;
    else if (text == "null") type = TokenType::Null;
    else if (text == "undefined") type = TokenType::Undefined;

    return Token{type, std::move(text), 0.0, tokenLine, tokenCol};
}

} // namespace browser::javascript

