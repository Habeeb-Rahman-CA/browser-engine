#pragma once

#include "browser/core/types.hpp"
#include <iosfwd>

namespace browser::layout {

struct EdgeSizes {
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};
    float left{0.0f};

    [[nodiscard]] constexpr float horizontal() const noexcept { return left + right; }
    [[nodiscard]] constexpr float vertical() const noexcept { return top + bottom; }
};

struct RectF {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    [[nodiscard]] constexpr bool contains(float px, float py) const noexcept {
        return px >= x && px < x + width &&
               py >= y && py < y + height;
    }

    [[nodiscard]] core::Rect toIntRect() const noexcept {
        return {
            static_cast<int>(x),
            static_cast<int>(y),
            static_cast<int>(width),
            static_cast<int>(height)
        };
    }
};

struct BoxDimensions {
    RectF content;
    EdgeSizes padding;
    EdgeSizes border;
    EdgeSizes margin;

    [[nodiscard]] RectF paddingBox() const noexcept {
        return {
            content.x - padding.left,
            content.y - padding.top,
            content.width + padding.horizontal(),
            content.height + padding.vertical()
        };
    }

    [[nodiscard]] RectF borderBox() const noexcept {
        auto p = paddingBox();
        return {
            p.x - border.left,
            p.y - border.top,
            p.width + border.horizontal(),
            p.height + border.vertical()
        };
    }

    [[nodiscard]] RectF marginBox() const noexcept {
        auto b = borderBox();
        return {
            b.x - margin.left,
            b.y - margin.top,
            b.width + margin.horizontal(),
            b.height + margin.vertical()
        };
    }
};

} // namespace browser::layout

