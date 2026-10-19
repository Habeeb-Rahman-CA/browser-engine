#pragma once

#include "browser/css/stylesheet.hpp"
#include "browser/style/computed_style.hpp"
#include "browser/dom/document.hpp"

#include <vector>
#include <memory>
#include <unordered_map>

namespace browser::style {

class StyleResolver {
public:
    StyleResolver();

    // Add author stylesheet
    void addAuthorStyleSheet(css::StyleSheet sheet);

    // Clear author stylesheets
    void clearAuthorStyleSheets();

    // Resolve computed style for an element
    [[nodiscard]] std::shared_ptr<ComputedStyle> resolveStyle(
        const dom::Element& element,
        const ComputedStyle* parentStyle = nullptr
    );

    // Recursively resolve computed styles for an entire DOM tree
    void resolveTree(
        const std::shared_ptr<dom::Node>& root,
        std::unordered_map<const dom::Node*, std::shared_ptr<ComputedStyle>>& styleMap,
        const ComputedStyle* parentStyle = nullptr
    );

    // Extract embedded <style> tags from Document and register them
    void loadStylesFromDocument(const dom::Document& document);

    [[nodiscard]] const css::StyleSheet& userAgentStyleSheet() const noexcept { return m_userAgentSheet; }

private:
    void initUserAgentStyleSheet();
    void applyDeclaration(ComputedStyle& style, const css::Declaration& decl);

    css::StyleSheet m_userAgentSheet;
    std::vector<css::StyleSheet> m_authorSheets;
};

} // namespace browser::style

