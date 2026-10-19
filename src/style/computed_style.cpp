#include "browser/style/computed_style.hpp"

#include <cctype>
#include <charconv>
#include <sstream>
#include <algorithm>

namespace browser::style {

std::ostream& operator<<(std::ostream& os, Display display) {
    switch (display) {
        case Display::Block:       return os << "block";
        case Display::Inline:      return os << "inline";
        case Display::InlineBlock: return os << "inline-block";
        case Display::None:        return os << "none";
    }
    return os << "unknown";
}

namespace {

std::string toLower(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (char c : str) res.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return res;
}

std::string trim(std::string_view s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        start++;
    }
    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end--;
    }
    return std::string(s.substr(start, end - start));
}

} // namespace

bool ComputedStyle::isInherited(std::string_view property) noexcept {
    static const std::string_view inheritedProps[] = {
        "color", "font-family", "font-size", "font-weight",
        "font-style", "line-height", "text-align", "visibility"
    };
    for (auto p : inheritedProps) {
        if (property == p) return true;
    }
    return false;
}

std::optional<core::Color> ComputedStyle::parseColor(std::string_view str) {
    std::string s = toLower(trim(str));
    if (s.empty()) return std::nullopt;

    // Named colors
    if (s == "transparent") return core::Color::transparent();
    if (s == "black")       return core::Color::black();
    if (s == "white")       return core::Color::white();
    if (s == "red")         return core::Color::fromRgb(255, 0, 0);
    if (s == "green")       return core::Color::fromRgb(0, 128, 0);
    if (s == "blue")        return core::Color::fromRgb(0, 0, 255);
    if (s == "gray")        return core::Color::fromRgb(128, 128, 128);
    if (s == "lightgray")   return core::Color::fromRgb(211, 211, 211);
    if (s == "yellow")      return core::Color::fromRgb(255, 255, 0);
    if (s == "cyan")        return core::Color::fromRgb(0, 255, 255);
    if (s == "magenta")     return core::Color::fromRgb(255, 0, 255);
    if (s == "orange")      return core::Color::fromRgb(255, 165, 0);

    // Hex colors
    if (s.starts_with("#")) {
        std::string hex = s.substr(1);
        if (hex.size() == 3) {
            // #RGB -> #RRGGBB
            uint32_t r = 0, g = 0, b = 0;
            std::from_chars(hex.data(), hex.data() + 1, r, 16);
            std::from_chars(hex.data() + 1, hex.data() + 2, g, 16);
            std::from_chars(hex.data() + 2, hex.data() + 3, b, 16);
            return core::Color::fromRgb(static_cast<uint8_t>(r * 17), static_cast<uint8_t>(g * 17), static_cast<uint8_t>(b * 17));
        } else if (hex.size() == 6) {
            uint32_t val = 0;
            auto res = std::from_chars(hex.data(), hex.data() + hex.size(), val, 16);
            if (res.ec == std::errc()) {
                return core::Color::fromRgb(
                    static_cast<uint8_t>((val >> 16) & 0xFF),
                    static_cast<uint8_t>((val >> 8) & 0xFF),
                    static_cast<uint8_t>(val & 0xFF)
                );
            }
        } else if (hex.size() == 8) {
            uint32_t val = 0;
            auto res = std::from_chars(hex.data(), hex.data() + hex.size(), val, 16);
            if (res.ec == std::errc()) {
                return core::Color::fromRgba(
                    static_cast<uint8_t>((val >> 24) & 0xFF),
                    static_cast<uint8_t>((val >> 16) & 0xFF),
                    static_cast<uint8_t>((val >> 8) & 0xFF),
                    static_cast<uint8_t>(val & 0xFF)
                );
            }
        }
    }

    // rgb(r, g, b) or rgba(r, g, b, a)
    if (s.starts_with("rgb(") || s.starts_with("rgba(")) {
        size_t openParen = s.find('(');
        size_t closeParen = s.find(')');
        if (openParen != std::string::npos && closeParen != std::string::npos && closeParen > openParen) {
            std::string inner = s.substr(openParen + 1, closeParen - openParen - 1);
            std::replace(inner.begin(), inner.end(), ',', ' ');
            std::istringstream ss(inner);
            int r = 0, g = 0, b = 0;
            float a = 1.0f;
            if (ss >> r >> g >> b) {
                if (ss >> a) {
                    return core::Color::fromRgba(
                        static_cast<uint8_t>(std::clamp(r, 0, 255)),
                        static_cast<uint8_t>(std::clamp(g, 0, 255)),
                        static_cast<uint8_t>(std::clamp(b, 0, 255)),
                        static_cast<uint8_t>(std::clamp(static_cast<int>(a * 255.0f), 0, 255))
                    );
                }
                return core::Color::fromRgb(
                    static_cast<uint8_t>(std::clamp(r, 0, 255)),
                    static_cast<uint8_t>(std::clamp(g, 0, 255)),
                    static_cast<uint8_t>(std::clamp(b, 0, 255))
                );
            }
        }
    }

    return std::nullopt;
}

Length ComputedStyle::parseLength(std::string_view str) {
    std::string s = toLower(trim(str));
    if (s.empty() || s == "auto") {
        return Length::autoLength();
    }

    if (s.ends_with("%")) {
        float val = std::stof(s.substr(0, s.size() - 1));
        return Length::percent(val);
    }

    if (s.ends_with("px")) {
        float val = std::stof(s.substr(0, s.size() - 2));
        return Length::px(val);
    }

    if (s.ends_with("pt")) {
        float val = std::stof(s.substr(0, s.size() - 2)) * 1.3333f; // 1pt ≈ 1.333px
        return Length::px(val);
    }

    // Unitless 0
    try {
        float val = std::stof(s);
        return Length::px(val);
    } catch (...) {
        return Length::autoLength();
    }
}

Display ComputedStyle::parseDisplay(std::string_view str) {
    std::string s = toLower(trim(str));
    if (s == "block")        return Display::Block;
    if (s == "inline-block") return Display::InlineBlock;
    if (s == "inline")       return Display::Inline;
    if (s == "none")         return Display::None;
    return Display::Inline;
}

TextAlign ComputedStyle::parseTextAlign(std::string_view str) {
    std::string s = toLower(trim(str));
    if (s == "center") return TextAlign::Center;
    if (s == "right")  return TextAlign::Right;
    return TextAlign::Left;
}

} // namespace browser::style
