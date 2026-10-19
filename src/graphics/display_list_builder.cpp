#include "browser/graphics/display_list_builder.hpp"
#include "browser/core/diagnostics.hpp"

namespace browser::graphics {

DisplayList DisplayListBuilder::build(const layout::LayoutBox& root) {
    core::ScopedTimer timer(core::Subsystem::DisplayList);
    DisplayList list;
    renderBox(root, list);
    core::BrowserDiagnostics::instance().incrementCounter("DisplayItemsEmitted", list.size());
    return list;
}

void DisplayListBuilder::renderBox(const layout::LayoutBox& box, DisplayList& list) {
    // 1. Background
    renderBackground(box, list);

    // 2. Borders
    renderBorders(box, list);

    // 3. Text
    renderText(box, list);

    // 4. Children
    for (const auto& child : box.children()) {
        renderBox(*child, list);
    }
}

void DisplayListBuilder::renderBackground(const layout::LayoutBox& box, DisplayList& list) {
    if (!box.style()) return;
    const auto& bg = box.style()->backgroundColor;
    if (bg.a > 0) {
        list.addFillRect(box.dimensions().paddingBox(), bg, box.style()->borderRadius);
    }
}

void DisplayListBuilder::renderBorders(const layout::LayoutBox& box, DisplayList& list) {
    if (!box.style()) return;
    const auto& b = box.style()->border;
    auto bBox = box.dimensions().borderBox();
    float r = box.style()->borderRadius;

    if (r > 0.0f) {
        if (b.top.width > 0.0f && b.top.color.a > 0 && b.top.style != style::BorderStyle::None) {
            list.addStrokeRect(bBox, b.top.color, b.top.width, r);
        }
        return;
    }

    // Top border
    if (b.top.width > 0.0f && b.top.color.a > 0 && b.top.style != style::BorderStyle::None) {
        list.addFillRect(layout::RectF{bBox.x, bBox.y, bBox.width, b.top.width}, b.top.color);
    }
    // Bottom border
    if (b.bottom.width > 0.0f && b.bottom.color.a > 0 && b.bottom.style != style::BorderStyle::None) {
        list.addFillRect(layout::RectF{bBox.x, bBox.y + bBox.height - b.bottom.width, bBox.width, b.bottom.width}, b.bottom.color);
    }
    // Left border
    if (b.left.width > 0.0f && b.left.color.a > 0 && b.left.style != style::BorderStyle::None) {
        list.addFillRect(layout::RectF{bBox.x, bBox.y, b.left.width, bBox.height}, b.left.color);
    }
    // Right border
    if (b.right.width > 0.0f && b.right.color.a > 0 && b.right.style != style::BorderStyle::None) {
        list.addFillRect(layout::RectF{bBox.x + bBox.width - b.right.width, bBox.y, b.right.width, bBox.height}, b.right.color);
    }
}

void DisplayListBuilder::renderText(const layout::LayoutBox& box, DisplayList& list) {
    if (box.text().empty()) return;

    core::Color textColor = box.style() ? box.style()->color : core::Color::black();
    float fontSize = box.style() ? box.style()->fontSize : 16.0f;

    list.addText(box.text(), fontSize, textColor, box.dimensions().content);
}

} // namespace browser::graphics

