#pragma once

#include "browser/graphics/display_list.hpp"
#include "browser/layout/layout_box.hpp"
#include "browser/core/types.hpp"

struct SDL_Renderer;

namespace browser::graphics {

class Rasterizer {
public:
    // Rasterize display list commands onto an SDL3 renderer with viewport offset and scroll position
    static void rasterize(
        SDL_Renderer* renderer,
        const DisplayList& displayList,
        const core::Rect& viewport,
        const core::Point& scrollOffset = {0, 0}
    );

    // Hit test: find the topmost element at screen coordinate `pos` given viewport and scroll offset
    [[nodiscard]] static const dom::Element* hitTest(
        const layout::LayoutBox& root,
        const core::Point& pos,
        const core::Rect& viewport,
        const core::Point& scrollOffset = {0, 0}
    );
};

} // namespace browser::graphics

