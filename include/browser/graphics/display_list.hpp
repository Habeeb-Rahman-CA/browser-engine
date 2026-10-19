#pragma once

#include "browser/graphics/display_item.hpp"
#include <vector>
#include <iosfwd>

namespace browser::graphics {

class DisplayList {
public:
    DisplayList() = default;

    void addFillRect(const layout::RectF& rect, const core::Color& color, float borderRadius = 0.0f);
    void addStrokeRect(const layout::RectF& rect, const core::Color& color, float width = 1.0f, float borderRadius = 0.0f);
    void addText(std::string text, float fontSize, const core::Color& color, const layout::RectF& bounds);
    void pushClip(const layout::RectF& clipRect);
    void popClip();

    void clear() noexcept { m_items.clear(); }
    [[nodiscard]] const std::vector<DisplayItem>& items() const noexcept { return m_items; }
    [[nodiscard]] bool empty() const noexcept { return m_items.empty(); }
    [[nodiscard]] size_t size() const noexcept { return m_items.size(); }

    void dump(std::ostream& out) const;
    [[nodiscard]] std::string toSvg(float width, float height) const;
    [[nodiscard]] std::string toJson() const;

private:
    std::vector<DisplayItem> m_items;
};

} // namespace browser::graphics

