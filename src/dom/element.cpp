#include "browser/dom/element.hpp"

#include <sstream>
#include <algorithm>

namespace browser::dom {

namespace {

std::string toLower(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

} // namespace

Element::Element(std::string tagName)
    : Node(NodeType::Element, toLower(tagName)) {}

bool Element::hasAttribute(std::string_view name) const noexcept {
    return m_attributes.find(std::string(name)) != m_attributes.end();
}

std::string Element::getAttribute(std::string_view name) const noexcept {
    auto it = m_attributes.find(std::string(name));
    if (it != m_attributes.end()) {
        return it->second;
    }
    return "";
}

void Element::setAttribute(std::string_view name, std::string_view value) {
    m_attributes[toLower(name)] = std::string(value);
}

void Element::removeAttribute(std::string_view name) {
    m_attributes.erase(toLower(name));
}

std::string Element::id() const noexcept {
    return getAttribute("id");
}

std::string Element::className() const noexcept {
    return getAttribute("class");
}

std::vector<std::string> Element::classList() const {
    std::vector<std::string> classes;
    std::istringstream ss(className());
    std::string token;
    while (ss >> token) {
        classes.push_back(token);
    }
    return classes;
}

std::shared_ptr<Element> Element::getElementById(std::string_view id) const {
    if (this->id() == id) {
        return std::const_pointer_cast<Element>(
            std::static_pointer_cast<const Element>(shared_from_this())
        );
    }

    for (const auto& child : m_children) {
        if (child->nodeType() == NodeType::Element) {
            auto elem = std::static_pointer_cast<Element>(child);
            if (auto found = elem->getElementById(id)) {
                return found;
            }
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Element>> Element::getElementsByTagName(std::string_view tagName) const {
    std::vector<std::shared_ptr<Element>> results;
    std::string targetTag = toLower(tagName);

    std::vector<std::shared_ptr<Node>> stack;
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        stack.push_back(*it);
    }

    while (!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();

        if (current->nodeType() == NodeType::Element) {
            auto elem = std::static_pointer_cast<Element>(current);
            if (targetTag == "*" || elem->tagName() == targetTag) {
                results.push_back(elem);
            }
            const auto& children = elem->childNodes();
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                stack.push_back(*it);
            }
        }
    }

    return results;
}

std::string Element::textContent() const {
    return Node::textContent();
}

void Element::setTextContent(std::string_view text) {
    Node::setTextContent(text);
}

} // namespace browser::dom

