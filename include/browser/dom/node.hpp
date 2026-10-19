#pragma once

#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <iosfwd>

namespace browser::dom {

enum class NodeType {
    Document,
    Element,
    Text,
    Comment
};

[[nodiscard]] std::string_view nodeTypeToString(NodeType type) noexcept;

class Element;

class Node : public std::enable_shared_from_this<Node> {
public:
    Node(NodeType type, std::string nodeName);
    virtual ~Node() = default;

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;

    [[nodiscard]] NodeType nodeType() const noexcept { return m_type; }
    [[nodiscard]] const std::string& nodeName() const noexcept { return m_nodeName; }

    // Tree navigation
    [[nodiscard]] std::shared_ptr<Node> parentNode() const noexcept;
    [[nodiscard]] const std::vector<std::shared_ptr<Node>>& childNodes() const noexcept { return m_children; }
    [[nodiscard]] std::shared_ptr<Node> firstChild() const noexcept;
    [[nodiscard]] std::shared_ptr<Node> lastChild() const noexcept;
    [[nodiscard]] std::shared_ptr<Node> previousSibling() const noexcept;
    [[nodiscard]] std::shared_ptr<Node> nextSibling() const noexcept;
    [[nodiscard]] bool hasChildNodes() const noexcept { return !m_children.empty(); }

    // Tree mutation
    void appendChild(std::shared_ptr<Node> child);
    void removeChild(std::shared_ptr<Node> child);
    void insertBefore(std::shared_ptr<Node> newChild, std::shared_ptr<Node> refChild);

    // Text representation
    [[nodiscard]] virtual std::string textContent() const;
    virtual void setTextContent(std::string_view text);

    // Tree inspection
    void dumpTree(std::ostream& out, int depth = 0) const;

protected:
    NodeType m_type;
    std::string m_nodeName;
    std::weak_ptr<Node> m_parent;
    std::vector<std::shared_ptr<Node>> m_children;
};

} // namespace browser::dom

