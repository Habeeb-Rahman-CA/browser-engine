#pragma once

#include "browser/graphics/display_list.hpp"
#include "browser/layout/layout_box.hpp"

namespace browser::graphics {

class DisplayListBuilder {
public:
    // Build display list from a layout box tree
    [[nodiscard]] static DisplayList build(const layout::LayoutBox& root);

private:
    static void renderBox(const layout::LayoutBox& box, DisplayList& list);
    static void renderBackground(const layout::LayoutBox& box, DisplayList& list);
    static void renderBorders(const layout::LayoutBox& box, DisplayList& list);
    static void renderText(const layout::LayoutBox& box, DisplayList& list);
};

} // namespace browser::graphics

