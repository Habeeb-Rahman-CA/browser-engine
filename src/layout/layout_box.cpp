#include "browser/layout/layout_box.hpp"
#include "browser/dom/element.hpp"

#include <iostream>
#include <iomanip>

namespace browser::layout {

std::string_view boxTypeToString(BoxType type) noexcept {
    switch (type) {
        case BoxType::BlockNode:      return "Block";
        case BoxType::InlineNode:     return "Inline";
        case BoxType::AnonymousBlock: return "AnonymousBlock";
    }
    return "Unknown";
}

LayoutBox::LayoutBox(BoxType type, std::shared_ptr<style::ComputedStyle> style, std::shared_ptr<dom::Node> node)
    : m_boxType(type), m_node(std::move(node)), m_style(std::move(style)) {}

void LayoutBox::addChild(std::unique_ptr<LayoutBox> child) {
    if (!child) return;
    child->m_parent = this;
    m_children.push_back(std::move(child));
}

void LayoutBox::dumpLayoutTree(std::ostream& out, int depth) const {
    for (int i = 0; i < depth; ++i) {
        out << "  ";
    }

    out << boxTypeToString(m_boxType);
    if (m_node) {
        out << " <" << m_node->nodeName() << ">";
    }
    if (!m_text.empty()) {
        out << " \"" << m_text << "\"";
    }

    out << " [pos: (" << m_dimensions.content.x << ", " << m_dimensions.content.y
        << "), size: " << m_dimensions.content.width << "x" << m_dimensions.content.height
        << " (border-box: " << m_dimensions.borderBox().width << "x" << m_dimensions.borderBox().height << ")]\n";

    for (const auto& child : m_children) {
        child->dumpLayoutTree(out, depth + 1);
    }
}

} // namespace browser::layout

