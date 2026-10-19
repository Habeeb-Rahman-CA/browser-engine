#include "browser/dom/node.hpp"
#include "browser/dom/text_node.hpp"

#include <algorithm>
#include <iostream>

namespace browser::dom {

std::string_view nodeTypeToString(NodeType type) noexcept {
    switch (type) {
        case NodeType::Document: return "Document";
        case NodeType::Element:  return "Element";
        case NodeType::Text:     return "Text";
        case NodeType::Comment:  return "Comment";
    }
    return "Unknown";
}

Node::Node(NodeType type, std::string nodeName)
    : m_type(type), m_nodeName(std::move(nodeName)) {}

std::shared_ptr<Node> Node::parentNode() const noexcept {
    return m_parent.lock();
}

std::shared_ptr<Node> Node::firstChild() const noexcept {
    if (m_children.empty()) return nullptr;
    return m_children.front();
}

std::shared_ptr<Node> Node::lastChild() const noexcept {
    if (m_children.empty()) return nullptr;
    return m_children.back();
}

std::shared_ptr<Node> Node::previousSibling() const noexcept {
    auto parent = m_parent.lock();
    if (!parent) return nullptr;

    const auto& siblings = parent->m_children;
    for (size_t i = 1; i < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            return siblings[i - 1];
        }
    }
    return nullptr;
}

std::shared_ptr<Node> Node::nextSibling() const noexcept {
    auto parent = m_parent.lock();
    if (!parent) return nullptr;

    const auto& siblings = parent->m_children;
    for (size_t i = 0; i + 1 < siblings.size(); ++i) {
        if (siblings[i].get() == this) {
            return siblings[i + 1];
        }
    }
    return nullptr;
}

void Node::appendChild(std::shared_ptr<Node> child) {
    if (!child) return;

    // Detach from previous parent if already attached
    if (auto prevParent = child->m_parent.lock()) {
        prevParent->removeChild(child);
    }

    child->m_parent = weak_from_this();
    m_children.push_back(std::move(child));
}

void Node::removeChild(std::shared_ptr<Node> child) {
    if (!child) return;

    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->m_parent.reset();
        m_children.erase(it);
    }
}

void Node::insertBefore(std::shared_ptr<Node> newChild, std::shared_ptr<Node> refChild) {
    if (!newChild) return;
    if (!refChild) {
        appendChild(std::move(newChild));
        return;
    }

    auto it = std::find(m_children.begin(), m_children.end(), refChild);
    if (it == m_children.end()) {
        appendChild(std::move(newChild));
        return;
    }

    if (auto prevParent = newChild->m_parent.lock()) {
        prevParent->removeChild(newChild);
        // Recalculate iterator in case removal shifted elements
        it = std::find(m_children.begin(), m_children.end(), refChild);
    }

    newChild->m_parent = weak_from_this();
    m_children.insert(it, std::move(newChild));
}

std::string Node::textContent() const {
    std::string text;
    for (const auto& child : m_children) {
        text += child->textContent();
    }
    return text;
}

void Node::setTextContent(std::string_view text) {
    // Clear existing children
    for (auto& child : m_children) {
        child->m_parent.reset();
    }
    m_children.clear();

    if (!text.empty()) {
        auto textNode = std::make_shared<TextNode>(std::string(text));
        appendChild(std::move(textNode));
    }
}

void Node::dumpTree(std::ostream& out, int depth) const {
    for (int i = 0; i < depth; ++i) {
        out << "  ";
    }

    out << nodeTypeToString(m_type) << " <" << m_nodeName << ">";
    if (m_type == NodeType::Text) {
        out << " \"" << textContent() << "\"";
    }
    out << "\n";

    for (const auto& child : m_children) {
        child->dumpTree(out, depth + 1);
    }
}

} // namespace browser::dom

