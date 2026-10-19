#include "browser/layout/layout_tree_builder.hpp"
#include "browser/core/diagnostics.hpp"

#include <algorithm>

namespace browser::layout {

namespace {

bool isAllWhitespace(std::string_view str) noexcept {
    return std::all_of(str.begin(), str.end(), [](char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    });
}

} // namespace

std::unique_ptr<LayoutBox> LayoutTreeBuilder::build(
    const std::shared_ptr<dom::Node>& root,
    const StyleMap& styleMap
) {
    if (!root) return nullptr;
    return buildRecursive(root, styleMap);
}

std::unique_ptr<LayoutBox> LayoutTreeBuilder::buildRecursive(
    const std::shared_ptr<dom::Node>& node,
    const StyleMap& styleMap
) {
    if (!node) return nullptr;

    // Handle Text nodes
    if (node->nodeType() == dom::NodeType::Text) {
        std::string text = node->textContent();
        if (isAllWhitespace(text)) {
            return nullptr;
        }

        // Inherit parent element style
        std::shared_ptr<style::ComputedStyle> parentStyle;
        if (auto parent = node->parentNode()) {
            auto it = styleMap.find(parent.get());
            if (it != styleMap.end()) {
                parentStyle = it->second;
            }
        }

        auto box = std::make_unique<LayoutBox>(BoxType::InlineNode, parentStyle, node);
        box->setText(std::move(text));
        core::BrowserDiagnostics::instance().incrementCounter("LayoutBoxesCreated");
        return box;
    }

    // Handle Elements
    if (node->nodeType() == dom::NodeType::Element) {
        auto it = styleMap.find(node.get());
        if (it == styleMap.end()) {
            return nullptr;
        }

        const auto& style = it->second;
        if (!style || style->display == style::Display::None) {
            return nullptr;
        }

        BoxType type = (style->display == style::Display::Block) ? BoxType::BlockNode : BoxType::InlineNode;
        auto box = std::make_unique<LayoutBox>(type, style, node);
        core::BrowserDiagnostics::instance().incrementCounter("LayoutBoxesCreated");

        std::vector<std::unique_ptr<LayoutBox>> childBoxes;
        for (const auto& child : node->childNodes()) {
            if (auto childBox = buildRecursive(child, styleMap)) {
                childBoxes.push_back(std::move(childBox));
            }
        }

        // If this is a block box, check if we need anonymous blocks
        bool hasBlockChildren = false;
        bool hasInlineChildren = false;
        for (const auto& child : childBoxes) {
            if (child->boxType() == BoxType::BlockNode) hasBlockChildren = true;
            if (child->boxType() == BoxType::InlineNode) hasInlineChildren = true;
        }

        if (hasBlockChildren && hasInlineChildren) {
            // Mixed block and inline children: wrap consecutive inline children in AnonymousBlock
            std::unique_ptr<LayoutBox> currentAnon = nullptr;

            for (auto& child : childBoxes) {
                if (child->boxType() == BoxType::InlineNode) {
                    if (!currentAnon) {
                        currentAnon = std::make_unique<LayoutBox>(BoxType::AnonymousBlock, style);
                        core::BrowserDiagnostics::instance().incrementCounter("LayoutBoxesCreated");
                    }
                    currentAnon->addChild(std::move(child));
                } else {
                    if (currentAnon) {
                        box->addChild(std::move(currentAnon));
                        currentAnon = nullptr;
                    }
                    box->addChild(std::move(child));
                }
            }
            if (currentAnon) {
                box->addChild(std::move(currentAnon));
            }
        } else {
            for (auto& child : childBoxes) {
                box->addChild(std::move(child));
            }
        }

        return box;
    }

    // For Document node, wrap children
    if (node->nodeType() == dom::NodeType::Document) {
        auto docBox = std::make_unique<LayoutBox>(BoxType::BlockNode);
        for (const auto& child : node->childNodes()) {
            if (auto childBox = buildRecursive(child, styleMap)) {
                docBox->addChild(std::move(childBox));
            }
        }
        if (docBox->children().empty()) return nullptr;
        return docBox;
    }

    return nullptr;
}

} // namespace browser::layout

