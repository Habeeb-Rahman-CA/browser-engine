#pragma once

#include "browser/html/token.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace browser::html {

enum class State {
    Data,
    CharacterReference,
    TagOpen,
    EndTagOpen,
    TagName,
    BeforeAttributeName,
    AttributeName,
    AfterAttributeName,
    BeforeAttributeValue,
    AttributeValueDoubleQuoted,
    AttributeValueSingleQuoted,
    AttributeValueUnquoted,
    AfterAttributeValueQuoted,
    SelfClosingStartTag,
    MarkupDeclarationOpen,
    CommentStart,
    CommentStartDash,
    Comment,
    CommentEndDash,
    CommentEnd,
    Doctype,
    BeforeDoctypeName,
    DoctypeName,
    AfterDoctypeName,
    BogusComment,
    BogusDoctype
};

class HtmlTokenizer {
public:
    explicit HtmlTokenizer(std::string_view input);

    // Pull next token
    HtmlToken nextToken();

    // Utility: tokenize entire input into a vector
    std::vector<HtmlToken> tokenizeAll();

    // Inspect current state and position
    [[nodiscard]] size_t currentLine() const noexcept { return m_line; }
    [[nodiscard]] size_t currentColumn() const noexcept { return m_column; }
    [[nodiscard]] bool isEof() const noexcept { return m_index >= m_input.size() && m_tokenQueue.empty(); }

    // Helper: is element tag a recognized HTML void element?
    [[nodiscard]] static bool isVoidElement(std::string_view tagName) noexcept;

private:
    char peek(size_t offset = 0) const noexcept;
    char advance() noexcept;
    bool match(std::string_view expected) noexcept;
    void emitToken(HtmlToken token);

    // Character reference resolution
    std::string consumeCharacterReference();

    std::string_view m_input;
    size_t m_index{0};
    size_t m_line{1};
    size_t m_column{1};

    State m_state{State::Data};
    HtmlToken m_currentToken;
    std::vector<HtmlToken> m_tokenQueue;

    // Temporary attribute being built
    std::string m_currentAttrName;
    std::string m_currentAttrVal;
};

} // namespace browser::html

