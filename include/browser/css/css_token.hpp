#pragma once

#include <string>
#include <string_view>
#include <iosfwd>

namespace browser::css {

enum class CssTokenType {
    Ident,
    String,
    Hash,
    Number,
    Dimension,
    Percentage,
    Whitespace,
    Colon,
    Semicolon,
    Comma,
    OpenBrace,
    CloseBrace,
    OpenParen,
    CloseParen,
    Delim,
    EndOfFile
};

[[nodiscard]] std::string_view cssTokenTypeToString(CssTokenType type) noexcept;

struct CssToken {
    CssTokenType type{CssTokenType::EndOfFile};
    std::string text;
    double numberValue{0.0};
    std::string unit;

    [[nodiscard]] std::string toString() const;
    bool operator==(const CssToken& other) const = default;
};

std::ostream& operator<<(std::ostream& os, const CssToken& token);

} // namespace browser::css

