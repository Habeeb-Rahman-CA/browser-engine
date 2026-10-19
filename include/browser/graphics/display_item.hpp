#pragma once

#include "browser/core/types.hpp"
#include "browser/layout/dimensions.hpp"

#include <string>
#include <variant>

namespace browser::graphics {

enum class ItemType {
    FillRect,
    StrokeRect,
    Text,
    PushClip,
    PopClip
};

struct FillRectItem {
    layout::RectF rect;
    core::Color color;
    float borderRadius{0.0f};
};

struct StrokeRectItem {
    layout::RectF rect;
    core::Color color;
    float strokeWidth{1.0f};
    float borderRadius{0.0f};
};

struct TextItem {
    std::string text;
    float fontSize{16.0f};
    core::Color color;
    layout::RectF bounds;
};

struct PushClipItem {
    layout::RectF clipRect;
};

struct PopClipItem {};

using ItemData = std::variant<
    FillRectItem,
    StrokeRectItem,
    TextItem,
    PushClipItem,
    PopClipItem
>;

struct DisplayItem {
    ItemType type;
    ItemData data;

    template <typename T>
    [[nodiscard]] const T* get() const noexcept {
        return std::get_if<T>(&data);
    }
};

} // namespace browser::graphics

