#pragma once

#include "browser/layout/layout_box.hpp"
#include "browser/dom/node.hpp"
#include "browser/style/computed_style.hpp"

#include <memory>
#include <unordered_map>

namespace browser::layout {

class LayoutTreeBuilder {
public:
    using StyleMap = std::unordered_map<const dom::Node*, std::shared_ptr<style::ComputedStyle>>;

    // Build the render/layout tree for a DOM document
    [[nodiscard]] static std::unique_ptr<LayoutBox> build(
        const std::shared_ptr<dom::Node>& root,
        const StyleMap& styleMap
    );

private:
    [[nodiscard]] static std::unique_ptr<LayoutBox> buildRecursive(
        const std::shared_ptr<dom::Node>& node,
        const StyleMap& styleMap
    );
};

} // namespace browser::layout

