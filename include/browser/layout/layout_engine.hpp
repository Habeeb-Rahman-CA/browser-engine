#pragma once

#include "browser/layout/layout_box.hpp"
#include "browser/core/types.hpp"

namespace browser::layout {

class LayoutEngine {
public:
    // Perform full layout calculation starting from the root layout box given viewport dimensions
    static void layout(LayoutBox& root, const core::Size& viewport);

private:
    static void layoutBox(LayoutBox& box, const BoxDimensions& containingBlock, float currentY = 0.0f);
    static void layoutBlock(LayoutBox& box, const BoxDimensions& containingBlock, float currentY = 0.0f);
    static void calculateBlockWidth(LayoutBox& box, const BoxDimensions& containingBlock);
    static void calculateBlockPosition(LayoutBox& box, const BoxDimensions& containingBlock, float currentY);
    static void layoutBlockChildren(LayoutBox& box);
    static void calculateBlockHeight(LayoutBox& box);
};

} // namespace browser::layout
