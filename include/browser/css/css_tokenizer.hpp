#pragma once

#include "browser/css/css_token.hpp"
#include <string_view>
#include <vector>

namespace browser::css {

class CssTokenizer {
public:
    explicit CssTokenizer(std::string_view input);

    CssToken nextToken();
    std::vector<CssToken> tokenizeAll();

    [[nodiscard]] bool isEof() const noexcept { return m_index >= m_input.size(); }

private:
    char peek(size_t offset = 0) const noexcept;
    char advance() noexcept;
    void skipWhitespaceAndComments();

    std::string_view m_input;
    size_t m_index{0};
};

} // namespace browser::css

