#include "browser/html/token.hpp"

#include <sstream>

namespace browser::html {

std::string_view tokenTypeToString(TokenType type) noexcept {
    switch (type) {
        case TokenType::DOCTYPE:    return "DOCTYPE";
        case TokenType::StartTag:   return "StartTag";
        case TokenType::EndTag:     return "EndTag";
        case TokenType::Comment:    return "Comment";
        case TokenType::Character:  return "Character";
        case TokenType::EndOfFile:  return "EndOfFile";
    }
    return "Unknown";
}

bool HtmlToken::hasAttribute(std::string_view name) const noexcept {
    for (const auto& attr : attributes) {
        if (attr.name == name) {
            return true;
        }
    }
    return false;
}

std::optional<std::string_view> HtmlToken::getAttribute(std::string_view name) const noexcept {
    for (const auto& attr : attributes) {
        if (attr.name == name) {
            return attr.value;
        }
    }
    return std::nullopt;
}

std::string HtmlToken::toString() const {
    std::ostringstream ss;
    ss << tokenTypeToString(type);

    switch (type) {
        case TokenType::DOCTYPE:
            ss << " (name: \"" << data << "\")";
            break;
        case TokenType::StartTag:
            ss << " <" << tagName;
            for (const auto& attr : attributes) {
                ss << " " << attr.name << "=\"" << attr.value << "\"";
            }
            if (selfClosing) {
                ss << " />";
            } else {
                ss << ">";
            }
            break;
        case TokenType::EndTag:
            ss << " </" << tagName << ">";
            break;
        case TokenType::Comment:
            ss << " <!--" << data << "-->";
            break;
        case TokenType::Character:
            ss << " \"" << data << "\"";
            break;
        case TokenType::EndOfFile:
            break;
    }
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const HtmlToken& token) {
    return os << token.toString();
}

} // namespace browser::html

