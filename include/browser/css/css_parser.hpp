#pragma once

#include "browser/css/stylesheet.hpp"
#include "browser/css/css_tokenizer.hpp"

#include <string_view>
#include <vector>
#include <optional>

namespace browser::css {

class CssParser {
public:
    explicit CssParser(std::string_view input);

    [[nodiscard]] StyleSheet parseStyleSheet();
    [[nodiscard]] std::vector<Declaration> parseDeclarationList();

    [[nodiscard]] static StyleSheet parse(std::string_view css);
    [[nodiscard]] static std::vector<Declaration> parseInline(std::string_view style);

private:
    const CssToken& peek(size_t offset = 0) const noexcept;
    const CssToken& advance() noexcept;
    void skipWhitespace();
    bool match(CssTokenType type) noexcept;

    std::vector<ComplexSelector> parseSelectorList();
    ComplexSelector parseComplexSelector();
    CompoundSelector parseCompoundSelector();
    std::vector<Declaration> parseBlockDeclarations();
    std::optional<Declaration> parseDeclaration();

    std::vector<CssToken> m_tokens;
    size_t m_index{0};
};

} // namespace browser::css
