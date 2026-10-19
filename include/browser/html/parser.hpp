#pragma once

#include "browser/html/tokenizer.hpp"
#include "browser/dom/document.hpp"

#include <memory>
#include <vector>
#include <string_view>

namespace browser::html {

enum class InsertionMode {
    Initial,
    BeforeHtml,
    BeforeHead,
    InHead,
    AfterHead,
    InBody,
    AfterBody
};

class HtmlParser {
public:
    explicit HtmlParser(std::string_view html);

    // Parse the entire document and return the root DOM Document
    [[nodiscard]] std::shared_ptr<dom::Document> parse();

    // Static helper
    [[nodiscard]] static std::shared_ptr<dom::Document> parseDocument(std::string_view html);

private:
    void handleToken(const HtmlToken& token);
    void handleDoctype(const HtmlToken& token);
    void handleStartTag(const HtmlToken& token);
    void handleEndTag(const HtmlToken& token);
    void handleCharacter(const HtmlToken& token);
    void handleComment(const HtmlToken& token);

    // Stack management & helpers
    [[nodiscard]] std::shared_ptr<dom::Element> currentElement() const;
    void pushElement(std::shared_ptr<dom::Element> element);
    void popElement();
    void popUntil(std::string_view tagName);
    [[nodiscard]] bool hasInScope(std::string_view tagName) const;

    void ensureHtmlElement();
    void ensureHeadElement();
    void ensureBodyElement();

    HtmlTokenizer m_tokenizer;
    std::shared_ptr<dom::Document> m_document;
    std::vector<std::shared_ptr<dom::Element>> m_openElements;
    InsertionMode m_insertionMode{InsertionMode::Initial};
};

} // namespace browser::html

