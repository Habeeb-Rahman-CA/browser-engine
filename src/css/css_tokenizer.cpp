#include "browser/css/css_tokenizer.hpp"
#include "browser/core/diagnostics.hpp"

#include <cctype>
#include <sstream>

namespace browser::css {

std::string_view cssTokenTypeToString(CssTokenType type) noexcept {
    switch (type) {
        case CssTokenType::Ident:       return "Ident";
        case CssTokenType::String:      return "String";
        case CssTokenType::Hash:        return "Hash";
        case CssTokenType::Number:      return "Number";
        case CssTokenType::Dimension:   return "Dimension";
        case CssTokenType::Percentage:  return "Percentage";
        case CssTokenType::Whitespace:  return "Whitespace";
        case CssTokenType::Colon:       return "Colon";
        case CssTokenType::Semicolon:   return "Semicolon";
        case CssTokenType::Comma:       return "Comma";
        case CssTokenType::OpenBrace:   return "OpenBrace";
        case CssTokenType::CloseBrace:  return "CloseBrace";
        case CssTokenType::OpenParen:   return "OpenParen";
        case CssTokenType::CloseParen:  return "CloseParen";
        case CssTokenType::Delim:       return "Delim";
        case CssTokenType::EndOfFile:   return "EndOfFile";
    }
    return "Unknown";
}

std::string CssToken::toString() const {
    std::ostringstream ss;
    ss << cssTokenTypeToString(type) << "(\"" << text << "\"";
    if (type == CssTokenType::Number || type == CssTokenType::Dimension || type == CssTokenType::Percentage) {
        ss << ", val=" << numberValue;
        if (!unit.empty()) ss << ", unit=" << unit;
    }
    ss << ")";
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const CssToken& token) {
    return os << token.toString();
}

CssTokenizer::CssTokenizer(std::string_view input)
    : m_input(input) {}

char CssTokenizer::peek(size_t offset) const noexcept {
    if (m_index + offset >= m_input.size()) return '\0';
    return m_input[m_index + offset];
}

char CssTokenizer::advance() noexcept {
    if (m_index >= m_input.size()) return '\0';
    return m_input[m_index++];
}

void CssTokenizer::skipWhitespaceAndComments() {
    while (m_index < m_input.size()) {
        char c = peek();
        if (c == '/' && peek(1) == '*') {
            // CSS comment /* ... */
            advance(); advance();
            while (m_index < m_input.size()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance();
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

CssToken CssTokenizer::nextToken() {
    core::ScopedTimer timer(core::Subsystem::CssTokenizer);

    skipWhitespaceAndComments();

    if (m_index >= m_input.size()) {
        return CssToken{CssTokenType::EndOfFile, "", 0.0, ""};
    }

    char c = peek();

    // Whitespace
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f') {
        std::string ws;
        while (m_index < m_input.size() && (peek() == ' ' || peek() == '\t' || peek() == '\r' || peek() == '\n' || peek() == '\f')) {
            ws.push_back(advance());
        }
        return CssToken{CssTokenType::Whitespace, ws, 0.0, ""};
    }

    // Punctuation
    switch (c) {
        case ':': advance(); return CssToken{CssTokenType::Colon, ":", 0.0, ""};
        case ';': advance(); return CssToken{CssTokenType::Semicolon, ";", 0.0, ""};
        case ',': advance(); return CssToken{CssTokenType::Comma, ",", 0.0, ""};
        case '{': advance(); return CssToken{CssTokenType::OpenBrace, "{", 0.0, ""};
        case '}': advance(); return CssToken{CssTokenType::CloseBrace, "}", 0.0, ""};
        case '(': advance(); return CssToken{CssTokenType::OpenParen, "(", 0.0, ""};
        case ')': advance(); return CssToken{CssTokenType::CloseParen, ")", 0.0, ""};
        default: break;
    }

    // Strings
    if (c == '"' || c == '\'') {
        char quote = advance();
        std::string str;
        while (m_index < m_input.size() && peek() != quote) {
            str.push_back(advance());
        }
        if (peek() == quote) advance();
        return CssToken{CssTokenType::String, str, 0.0, ""};
    }

    // Hash (colors or ID selectors)
    if (c == '#') {
        advance();
        std::string hash;
        while (m_index < m_input.size() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '-' || peek() == '_')) {
            hash.push_back(advance());
        }
        return CssToken{CssTokenType::Hash, hash, 0.0, ""};
    }

    // Numbers, Dimensions (px, pt, rem, em), and Percentages
    if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) ||
        (c == '-' && (std::isdigit(static_cast<unsigned char>(peek(1))) || (peek(1) == '.' && std::isdigit(static_cast<unsigned char>(peek(2))))))) {
        std::string numStr;
        if (c == '-') numStr.push_back(advance());

        while (m_index < m_input.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
            numStr.push_back(advance());
        }
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
            numStr.push_back(advance()); // '.'
            while (m_index < m_input.size() && std::isdigit(static_cast<unsigned char>(peek()))) {
                numStr.push_back(advance());
            }
        }

        double val = std::stod(numStr);

        if (peek() == '%') {
            advance();
            return CssToken{CssTokenType::Percentage, numStr + "%", val, "%"};
        }

        // Check for unit dimension
        if (std::isalpha(static_cast<unsigned char>(peek()))) {
            std::string unit;
            while (m_index < m_input.size() && std::isalpha(static_cast<unsigned char>(peek()))) {
                unit.push_back(advance());
            }
            return CssToken{CssTokenType::Dimension, numStr + unit, val, unit};
        }

        return CssToken{CssTokenType::Number, numStr, val, ""};
    }

    // Identifiers (keywords, properties, tag names)
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
        std::string ident;
        while (m_index < m_input.size() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '-' || peek() == '_')) {
            ident.push_back(advance());
        }
        return CssToken{CssTokenType::Ident, ident, 0.0, ""};
    }

    // Delimiters (>, +, ~, ., *)
    std::string delim(1, advance());
    return CssToken{CssTokenType::Delim, delim, 0.0, ""};
}

std::vector<CssToken> CssTokenizer::tokenizeAll() {
    std::vector<CssToken> tokens;
    while (true) {
        auto tok = nextToken();
        if (tok.type == CssTokenType::EndOfFile) {
            tokens.push_back(std::move(tok));
            break;
        }
        tokens.push_back(std::move(tok));
    }
    return tokens;
}

} // namespace browser::css

