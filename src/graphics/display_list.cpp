#include "browser/graphics/display_list.hpp"
#include "browser/core/serializer.hpp"

#include <iostream>

namespace browser::graphics {

void DisplayList::addFillRect(const layout::RectF& rect, const core::Color& color, float borderRadius) {
    if (color.a == 0 || rect.width <= 0.0f || rect.height <= 0.0f) return;
    m_items.push_back(DisplayItem{
        ItemType::FillRect,
        FillRectItem{rect, color, borderRadius}
    });
}

void DisplayList::addStrokeRect(const layout::RectF& rect, const core::Color& color, float width, float borderRadius) {
    if (color.a == 0 || width <= 0.0f) return;
    m_items.push_back(DisplayItem{
        ItemType::StrokeRect,
        StrokeRectItem{rect, color, width, borderRadius}
    });
}

void DisplayList::addText(std::string text, float fontSize, const core::Color& color, const layout::RectF& bounds) {
    if (text.empty() || color.a == 0) return;
    m_items.push_back(DisplayItem{
        ItemType::Text,
        TextItem{std::move(text), fontSize, color, bounds}
    });
}

void DisplayList::pushClip(const layout::RectF& clipRect) {
    m_items.push_back(DisplayItem{
        ItemType::PushClip,
        PushClipItem{clipRect}
    });
}

void DisplayList::popClip() {
    m_items.push_back(DisplayItem{
        ItemType::PopClip,
        PopClipItem{}
    });
}

void DisplayList::dump(std::ostream& out) const {
    out << "=== DISPLAY LIST (" << m_items.size() << " commands) ===\n";
    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        out << "  [" << i << "] ";
        switch (item.type) {
            case ItemType::FillRect: {
                const auto* d = item.get<FillRectItem>();
                out << "FillRect (" << d->rect.x << ", " << d->rect.y << ", " << d->rect.width << "x" << d->rect.height
                    << "), rgba(" << int(d->color.r) << "," << int(d->color.g) << "," << int(d->color.b) << "," << int(d->color.a) << ")";
                if (d->borderRadius > 0.0f) out << ", radius=" << d->borderRadius;
                out << "\n";
                break;
            }
            case ItemType::StrokeRect: {
                const auto* d = item.get<StrokeRectItem>();
                out << "StrokeRect (" << d->rect.x << ", " << d->rect.y << ", " << d->rect.width << "x" << d->rect.height
                    << "), width=" << d->strokeWidth;
                if (d->borderRadius > 0.0f) out << ", radius=" << d->borderRadius;
                out << "\n";
                break;
            }
            case ItemType::Text: {
                const auto* d = item.get<TextItem>();
                out << "Text \"" << d->text << "\" (" << d->bounds.x << ", " << d->bounds.y << "), size=" << d->fontSize << "px\n";
                break;
            }
            case ItemType::PushClip: {
                const auto* d = item.get<PushClipItem>();
                out << "PushClip (" << d->clipRect.x << ", " << d->clipRect.y << ", " << d->clipRect.width << "x" << d->clipRect.height << ")\n";
                break;
            }
            case ItemType::PopClip:
                out << "PopClip\n";
                break;
        }
    }
}

std::string DisplayList::toSvg(float width, float height) const {
    return core::Serializer::displayListToSvg(*this, width, height);
}

std::string DisplayList::toJson() const {
    return core::Serializer::displayListToJson(*this);
}

} // namespace browser::graphics

