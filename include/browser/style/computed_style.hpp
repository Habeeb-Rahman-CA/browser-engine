#pragma once

#include "browser/core/types.hpp"
#include <string>
#include <string_view>
#include <optional>

namespace browser::style {

enum class Display {
    Block,
    Inline,
    InlineBlock,
    None
};

std::ostream& operator<<(std::ostream& os, Display display);

enum class LengthUnit {
    Px,
    Percent,
    Auto
};

struct Length {
    float value{0.0f};
    LengthUnit unit{LengthUnit::Auto};

    [[nodiscard]] bool isAuto() const noexcept { return unit == LengthUnit::Auto; }
    [[nodiscard]] bool isPx() const noexcept { return unit == LengthUnit::Px; }
    [[nodiscard]] bool isPercent() const noexcept { return unit == LengthUnit::Percent; }

    static Length px(float val) noexcept { return {val, LengthUnit::Px}; }
    static Length percent(float val) noexcept { return {val, LengthUnit::Percent}; }
    static Length autoLength() noexcept { return {0.0f, LengthUnit::Auto}; }
};

struct BoxEdges {
    Length top{Length::px(0.0f)};
    Length right{Length::px(0.0f)};
    Length bottom{Length::px(0.0f)};
    Length left{Length::px(0.0f)};
};

enum class BorderStyle {
    None,
    Solid,
    Dashed,
    Dotted
};

struct BorderEdge {
    float width{0.0f};
    BorderStyle style{BorderStyle::None};
    core::Color color{core::Color::black()};
};

struct BoxBorders {
    BorderEdge top;
    BorderEdge right;
    BorderEdge bottom;
    BorderEdge left;
};

enum class TextAlign {
    Left,
    Center,
    Right
};

class ComputedStyle {
public:
    ComputedStyle() = default;

    // Display
    Display display{Display::Inline};

    // Dimensions
    Length width{Length::autoLength()};
    Length height{Length::autoLength()};

    // Box model
    BoxEdges margin;
    BoxEdges padding;
    BoxBorders border;
    float borderRadius{0.0f};

    // Colors
    core::Color color{core::Color::black()};
    core::Color backgroundColor{core::Color::transparent()};

    // Typography
    float fontSize{16.0f};
    uint16_t fontWeight{400}; // 400 = normal, 700 = bold
    std::string fontFamily{"sans-serif"};
    TextAlign textAlign{TextAlign::Left};

    // Opacity
    float opacity{1.0f};

    // Check if property naturally inherits from parent element in CSS
    [[nodiscard]] static bool isInherited(std::string_view property) noexcept;

    // Value parsers
    static std::optional<core::Color> parseColor(std::string_view str);
    static Length parseLength(std::string_view str);
    static Display parseDisplay(std::string_view str);
    static TextAlign parseTextAlign(std::string_view str);
};

} // namespace browser::style
