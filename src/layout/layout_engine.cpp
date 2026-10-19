#include "browser/layout/layout_engine.hpp"
#include "browser/core/diagnostics.hpp"

#include <algorithm>

namespace browser::layout {

void LayoutEngine::layout(LayoutBox& root, const core::Size& viewport) {
    core::ScopedTimer timer(core::Subsystem::Layout);

    BoxDimensions viewportBlock;
    viewportBlock.content.x = 0.0f;
    viewportBlock.content.y = 0.0f;
    viewportBlock.content.width = static_cast<float>(viewport.width);
    viewportBlock.content.height = static_cast<float>(viewport.height);

    layoutBox(root, viewportBlock);
}

void LayoutEngine::layoutBox(LayoutBox& box, const BoxDimensions& containingBlock, float currentY) {
    switch (box.boxType()) {
        case BoxType::BlockNode:
        case BoxType::AnonymousBlock:
            layoutBlock(box, containingBlock, currentY);
            break;

        case BoxType::InlineNode: {
            // Basic inline measurement
            auto& dims = box.dimensions();
            dims.content.x = containingBlock.content.x;
            dims.content.y = containingBlock.content.y + currentY;

            float fontSize = box.style() ? box.style()->fontSize : 16.0f;
            if (!box.text().empty()) {
                dims.content.width = static_cast<float>(box.text().size()) * (fontSize * 0.55f);
                dims.content.height = fontSize * 1.2f;
            } else {
                dims.content.width = fontSize;
                dims.content.height = fontSize * 1.2f;
            }

            // Recurse children
            for (auto& child : box.children()) {
                layoutBox(*child, dims, 0.0f);
            }
            break;
        }
    }
}

void LayoutEngine::layoutBlock(LayoutBox& box, const BoxDimensions& containingBlock, float currentY) {
    // 1. Calculate width and horizontal margins/padding/border
    calculateBlockWidth(box, containingBlock);

    // 2. Position this box within its containing block using resolved margins
    calculateBlockPosition(box, containingBlock, currentY);

    // 3. Layout children
    layoutBlockChildren(box);

    // 4. Calculate content height based on children or explicit height
    calculateBlockHeight(box);
}

void LayoutEngine::calculateBlockWidth(LayoutBox& box, const BoxDimensions& containingBlock) {
    auto& dims = box.dimensions();
    const auto* style = box.style().get();

    float containingWidth = containingBlock.content.width;

    // Resolve padding
    if (style) {
        dims.padding.top = style->padding.top.value;
        dims.padding.right = style->padding.right.value;
        dims.padding.bottom = style->padding.bottom.value;
        dims.padding.left = style->padding.left.value;

        dims.border.top = style->border.top.width;
        dims.border.right = style->border.right.width;
        dims.border.bottom = style->border.bottom.width;
        dims.border.left = style->border.left.width;
    }

    // Resolve margin
    float marginLeft = style ? style->margin.left.value : 0.0f;
    float marginRight = style ? style->margin.right.value : 0.0f;
    bool autoMarginLeft = style && style->margin.left.isAuto();
    bool autoMarginRight = style && style->margin.right.isAuto();

    if (style) {
        dims.margin.top = style->margin.top.value;
        dims.margin.bottom = style->margin.bottom.value;
    }

    // Resolve width
    float contentWidth = 0.0f;
    if (style && style->width.isPx()) {
        contentWidth = style->width.value;
    } else if (style && style->width.isPercent()) {
        contentWidth = (style->width.value * containingWidth) / 100.0f;
    } else {
        // Auto width takes available space minus margins/padding/borders
        float nonContentWidth = dims.padding.horizontal() + dims.border.horizontal();
        if (!autoMarginLeft) nonContentWidth += marginLeft;
        if (!autoMarginRight) nonContentWidth += marginRight;

        contentWidth = std::max(0.0f, containingWidth - nonContentWidth);
    }

    // Auto margin centering: e.g. margin: 0 auto
    float totalSpacing = containingWidth - (contentWidth + dims.padding.horizontal() + dims.border.horizontal());
    if (autoMarginLeft && autoMarginRight) {
        marginLeft = marginRight = std::max(0.0f, totalSpacing / 2.0f);
    } else if (autoMarginLeft) {
        marginLeft = std::max(0.0f, totalSpacing - marginRight);
    } else if (autoMarginRight) {
        marginRight = std::max(0.0f, totalSpacing - marginLeft);
    }

    dims.margin.left = marginLeft;
    dims.margin.right = marginRight;
    dims.content.width = contentWidth;
}

void LayoutEngine::calculateBlockPosition(LayoutBox& box, const BoxDimensions& containingBlock, float currentY) {
    auto& dims = box.dimensions();

    dims.content.x = containingBlock.content.x + dims.margin.left + dims.border.left + dims.padding.left;
    dims.content.y = containingBlock.content.y + currentY + dims.margin.top + dims.border.top + dims.padding.top;
}

void LayoutEngine::layoutBlockChildren(LayoutBox& box) {
    auto& dims = box.dimensions();
    float currentY = 0.0f;

    for (auto& child : box.children()) {
        layoutBox(*child, dims, currentY);

        // Advance vertical offset by child's margin-box height
        currentY += child->dimensions().marginBox().height;
    }
}

void LayoutEngine::calculateBlockHeight(LayoutBox& box) {
    auto& dims = box.dimensions();
    const auto* style = box.style().get();

    if (style && style->height.isPx()) {
        dims.content.height = style->height.value;
    } else if (style && style->height.isPercent() && box.parent()) {
        dims.content.height = (style->height.value * box.parent()->dimensions().content.height) / 100.0f;
    } else {
        // Auto height: sum of children heights
        float totalHeight = 0.0f;
        for (const auto& child : box.children()) {
            totalHeight += child->dimensions().marginBox().height;
        }
        dims.content.height = totalHeight;
    }
}

} // namespace browser::layout
