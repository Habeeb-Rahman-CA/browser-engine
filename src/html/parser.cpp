#include "browser/html/parser.hpp"
#include "browser/core/diagnostics.hpp"

#include <algorithm>

namespace browser::html {

namespace {

bool isFormattingOrBlockElement(std::string_view tag) noexcept {
    static const std::string_view blockTags[] = {
        "address", "article", "aside", "blockquote", "center", "details",
        "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure",
        "footer", "header", "hgroup", "main", "menu", "nav", "ol", "p",
        "section", "summary", "ul", "h1", "h2", "h3", "h4", "h5", "h6",
        "pre", "table"
    };
    for (auto b : blockTags) {
        if (tag == b) return true;
    }
    return false;
}

} // namespace

HtmlParser::HtmlParser(std::string_view html)
    : m_tokenizer(html),
      m_document(std::make_shared<dom::Document>()) {}

std::shared_ptr<dom::Document> HtmlParser::parse() {
    core::ScopedTimer timer(core::Subsystem::HtmlParser);
    core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated"); // for document

    while (!m_tokenizer.isEof()) {
        auto token = m_tokenizer.nextToken();
        if (token.type == TokenType::EndOfFile) {
            break;
        }
        handleToken(token);
    }

    return m_document;
}

std::shared_ptr<dom::Document> HtmlParser::parseDocument(std::string_view html) {
    HtmlParser parser(html);
    return parser.parse();
}

std::shared_ptr<dom::Element> HtmlParser::currentElement() const {
    if (m_openElements.empty()) {
        return nullptr;
    }
    return m_openElements.back();
}

void HtmlParser::pushElement(std::shared_ptr<dom::Element> element) {
    m_openElements.push_back(element);
}

void HtmlParser::popElement() {
    if (!m_openElements.empty()) {
        m_openElements.pop_back();
    }
}

void HtmlParser::popUntil(std::string_view tagName) {
    while (!m_openElements.empty()) {
        if (m_openElements.back()->tagName() == tagName) {
            popElement();
            break;
        }
        popElement();
    }
}

bool HtmlParser::hasInScope(std::string_view tagName) const {
    for (auto it = m_openElements.rbegin(); it != m_openElements.rend(); ++it) {
        if ((*it)->tagName() == tagName) {
            return true;
        }
        if ((*it)->tagName() == "html") {
            break;
        }
    }
    return false;
}

void HtmlParser::ensureHtmlElement() {
    if (!m_document->documentElement()) {
        auto html = m_document->createElement("html");
        m_document->appendChild(html);
        m_document->setDocumentElement(html);
        pushElement(html);
        core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated");
    }
}

void HtmlParser::ensureHeadElement() {
    ensureHtmlElement();
    if (!m_document->head()) {
        auto head = m_document->createElement("head");
        m_document->documentElement()->appendChild(head);
        m_document->setHead(head);
        pushElement(head);
        m_insertionMode = InsertionMode::InHead;
        core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated");
    }
}

void HtmlParser::ensureBodyElement() {
    ensureHtmlElement();
    if (m_insertionMode == InsertionMode::InHead) {
        popUntil("head");
        m_insertionMode = InsertionMode::AfterHead;
    }
    if (!m_document->body()) {
        auto body = m_document->createElement("body");
        m_document->documentElement()->appendChild(body);
        m_document->setBody(body);
        pushElement(body);
        m_insertionMode = InsertionMode::InBody;
        core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated");
    }
}

void HtmlParser::handleToken(const HtmlToken& token) {
    switch (token.type) {
        case TokenType::DOCTYPE:
            handleDoctype(token);
            break;
        case TokenType::StartTag:
            handleStartTag(token);
            break;
        case TokenType::EndTag:
            handleEndTag(token);
            break;
        case TokenType::Character:
            handleCharacter(token);
            break;
        case TokenType::Comment:
            handleComment(token);
            break;
        case TokenType::EndOfFile:
            break;
    }
}

void HtmlParser::handleDoctype(const HtmlToken& /*token*/) {
    // In our DOM, doctype initializes document mode
    m_insertionMode = InsertionMode::BeforeHtml;
}

void HtmlParser::handleStartTag(const HtmlToken& token) {
    const auto& tag = token.tagName;

    if (tag == "html") {
        ensureHtmlElement();
        // Copy any attributes to documentElement
        for (const auto& attr : token.attributes) {
            m_document->documentElement()->setAttribute(attr.name, attr.value);
        }
        m_insertionMode = InsertionMode::BeforeHead;
        return;
    }

    if (tag == "head") {
        ensureHeadElement();
        for (const auto& attr : token.attributes) {
            m_document->head()->setAttribute(attr.name, attr.value);
        }
        return;
    }

    if (tag == "body") {
        ensureBodyElement();
        for (const auto& attr : token.attributes) {
            m_document->body()->setAttribute(attr.name, attr.value);
        }
        return;
    }

    if (tag == "title" || tag == "meta" || tag == "link" || tag == "style" || tag == "script") {
        if (!m_document->head() && m_insertionMode != InsertionMode::InBody) {
            ensureHeadElement();
        }
    } else {
        if (!m_document->body()) {
            ensureBodyElement();
        }
    }

    // Auto-close open <p> when starting another block or paragraph element
    if (tag == "p" || isFormattingOrBlockElement(tag)) {
        if (hasInScope("p")) {
            popUntil("p");
        }
    }

    // Create DOM element
    auto elem = m_document->createElement(tag);
    for (const auto& attr : token.attributes) {
        elem->setAttribute(attr.name, attr.value);
    }
    core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated");

    // Attach to current parent
    if (auto parent = currentElement()) {
        parent->appendChild(elem);
    } else if (m_document->documentElement()) {
        m_document->documentElement()->appendChild(elem);
    } else {
        m_document->appendChild(elem);
    }

    // Special element registration
    if (tag == "body" && !m_document->body()) {
        m_document->setBody(elem);
    }

    // Void elements do not get pushed to the open elements stack
    if (!token.selfClosing && !HtmlTokenizer::isVoidElement(tag)) {
        pushElement(elem);
    }
}

void HtmlParser::handleEndTag(const HtmlToken& token) {
    const auto& tag = token.tagName;

    if (tag == "p") {
        if (!hasInScope("p")) {
            // Implicit open <p> then close
            handleStartTag(HtmlToken{TokenType::StartTag, "p", false, {}, "", "", "", false, 0, 0});
        }
        popUntil("p");
        return;
    }

    if (tag == "head") {
        popUntil("head");
        m_insertionMode = InsertionMode::AfterHead;
        return;
    }

    if (tag == "body") {
        popUntil("body");
        m_insertionMode = InsertionMode::AfterBody;
        return;
    }

    if (tag == "html") {
        m_insertionMode = InsertionMode::AfterBody;
        return;
    }

    if (hasInScope(tag)) {
        popUntil(tag);
    }
}

void HtmlParser::handleCharacter(const HtmlToken& token) {
    if (token.data.empty()) return;

    auto parent = currentElement();
    if (parent && (parent->tagName() == "title" || parent->tagName() == "style" || parent->tagName() == "script")) {
        if (auto lastChild = parent->lastChild()) {
            if (lastChild->nodeType() == dom::NodeType::Text) {
                auto textNode = std::static_pointer_cast<dom::TextNode>(lastChild);
                textNode->setData(textNode->data() + token.data);
                return;
            }
        }
        auto textNode = m_document->createTextNode(token.data);
        parent->appendChild(std::move(textNode));
        core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated");
        return;
    }

    // Check if whitespace before body
    bool isAllWs = std::all_of(token.data.begin(), token.data.end(), [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    });

    if (isAllWs && !m_document->body()) {
        return;
    }

    ensureBodyElement();

    parent = currentElement();
    if (!parent) {
        parent = m_document->body();
    }

    if (parent) {
        // Coalesce text if last child is already a text node
        if (auto lastChild = parent->lastChild()) {
            if (lastChild->nodeType() == dom::NodeType::Text) {
                auto textNode = std::static_pointer_cast<dom::TextNode>(lastChild);
                textNode->setData(textNode->data() + token.data);
                return;
            }
        }

        auto textNode = m_document->createTextNode(token.data);
        parent->appendChild(std::move(textNode));
        core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated");
    }
}

void HtmlParser::handleComment(const HtmlToken& token) {
    auto comment = m_document->createComment(token.data);
    core::BrowserDiagnostics::instance().incrementCounter("DomNodesCreated");

    if (auto parent = currentElement()) {
        parent->appendChild(std::move(comment));
    } else {
        m_document->appendChild(std::move(comment));
    }
}

} // namespace browser::html
