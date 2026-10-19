#pragma once

#include "browser/dom/node.hpp"
#include "browser/dom/element.hpp"
#include "browser/dom/text_node.hpp"
#include "browser/dom/comment_node.hpp"

namespace browser::dom {

class Document : public Node {
public:
    Document();

    // Factory methods
    [[nodiscard]] std::shared_ptr<Element> createElement(std::string_view tagName);
    [[nodiscard]] std::shared_ptr<TextNode> createTextNode(std::string_view data);
    [[nodiscard]] std::shared_ptr<CommentNode> createComment(std::string_view data);

    // Root pointers
    [[nodiscard]] std::shared_ptr<Element> documentElement() const noexcept { return m_documentElement; }
    void setDocumentElement(std::shared_ptr<Element> el) noexcept { m_documentElement = el; }

    [[nodiscard]] std::shared_ptr<Element> head() const noexcept { return m_head; }
    void setHead(std::shared_ptr<Element> head) noexcept { m_head = head; }

    [[nodiscard]] std::shared_ptr<Element> body() const noexcept { return m_body; }
    void setBody(std::shared_ptr<Element> body) noexcept { m_body = body; }

    [[nodiscard]] std::string title() const;
    void setTitle(std::string_view title);

    // Document querying
    [[nodiscard]] std::shared_ptr<Element> getElementById(std::string_view id) const;
    [[nodiscard]] std::vector<std::shared_ptr<Element>> getElementsByTagName(std::string_view tagName) const;

private:
    std::shared_ptr<Element> m_documentElement;
    std::shared_ptr<Element> m_head;
    std::shared_ptr<Element> m_body;
};

} // namespace browser::dom

