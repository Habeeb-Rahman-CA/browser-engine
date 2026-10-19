#pragma once

#include "browser/javascript/token.hpp"
#include <string_view>
#include <vector>

namespace browser::javascript {

class Lexer {
public:
    explicit Lexer(std::string_view source);

    [[nodiscard]] std::vector<Token> tokenize();

private:
    [[nodiscard]] bool isAtEnd() const noexcept;
    char advance();
    [[nodiscard]] char peek() const noexcept;
    [[nodiscard]] char peekNext() const noexcept;
    bool match(char expected);

    void skipWhitespaceAndComments();
    Token scanToken();
    Token scanString(char quote);
    Token scanNumber();
    Token scanIdentifierOrKeyword();

    std::string_view m_source;
    size_t m_start{0};
    size_t m_current{0};
    size_t m_line{1};
    size_t m_column{1};
};

} // namespace browser::javascript

