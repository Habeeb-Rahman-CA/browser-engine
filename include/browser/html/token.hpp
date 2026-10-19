#pragma once

#include <string>
#include <vector>
#include <optional>
#include <string_view>
#include <iosfwd>

namespace browser::html {

enum class TokenType {
    DOCTYPE,
    StartTag,
    EndTag,
    Comment,
    Character,
    EndOfFile
};

[[nodiscard]] std::string_view tokenTypeToString(TokenType type) noexcept;

struct Attribute {
    std::string name;
    std::string value;

    bool operator==(const Attribute& other) const = default;
};

struct HtmlToken {
    TokenType type{TokenType::EndOfFile};
    std::string tagName;           // For StartTag and EndTag (normalized to lowercase)
    bool selfClosing{false};       // Self-closing flag (e.g. <img /> or void element)
    std::vector<Attribute> attributes;
    std::string data;              // For Character, Comment, or DOCTYPE name
    std::string publicIdentifier;  // For DOCTYPE
    std::string systemIdentifier;  // For DOCTYPE
    bool forceQuirks{false};       // For DOCTYPE

    // Position metadata for diagnostics
    size_t line{1};
    size_t column{1};

    [[nodiscard]] bool hasAttribute(std::string_view name) const noexcept;
    [[nodiscard]] std::optional<std::string_view> getAttribute(std::string_view name) const noexcept;
    [[nodiscard]] std::string toString() const;

    bool operator==(const HtmlToken& other) const = default;
};

std::ostream& operator<<(std::ostream& os, const HtmlToken& token);

} // namespace browser::html

