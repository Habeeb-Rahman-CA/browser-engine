#include "browser/dom/document.hpp"

namespace browser::dom {

Document::Document()
    : Node(NodeType::Document, "#document") {}

std::shared_ptr<Element> Document::createElement(std::string_view tagName) {
    return std::make_shared<Element>(std::string(tagName));
}

std::shared_ptr<TextNode> Document::createTextNode(std::string_view data) {
    return std::make_shared<TextNode>(std::string(data));
}

std::shared_ptr<CommentNode> Document::createComment(std::string_view data) {
    return std::make_shared<CommentNode>(std::string(data));
}

std::string Document::title() const {
    if (m_head) {
        auto titleElements = m_head->getElementsByTagName("title");
        if (!titleElements.empty()) {
            return titleElements[0]->textContent();
        }
    }
    return "";
}

void Document::setTitle(std::string_view title) {
    if (!m_head) return;
    auto titleElements = m_head->getElementsByTagName("title");
    if (!titleElements.empty()) {
        titleElements[0]->setTextContent(title);
    } else {
        auto titleEl = createElement("title");
        titleEl->setTextContent(title);
        m_head->appendChild(std::move(titleEl));
    }
}

std::shared_ptr<Element> Document::getElementById(std::string_view id) const {
    if (m_documentElement) {
        return m_documentElement->getElementById(id);
    }
    return nullptr;
}

std::vector<std::shared_ptr<Element>> Document::getElementsByTagName(std::string_view tagName) const {
    if (m_documentElement) {
        auto results = m_documentElement->getElementsByTagName(tagName);
        if (tagName == "*" || m_documentElement->tagName() == tagName) {
            results.insert(results.begin(), m_documentElement);
        }
        return results;
    }
    return {};
}

} // namespace browser::dom

