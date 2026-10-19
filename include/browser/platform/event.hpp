#pragma once

#include "browser/core/types.hpp"
#include <string>
#include <variant>
#include <cstdint>

namespace browser::platform {

enum class EventType {
    Quit,
    WindowResize,
    WindowFocus,
    MouseMove,
    MouseButtonDown,
    MouseButtonUp,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput
};

enum class MouseButton {
    Left,
    Middle,
    Right,
    Other
};

struct KeyModifiers {
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
    bool super{false};
};

struct QuitEvent {};

struct WindowResizeEvent {
    core::Size newSize;
};

struct WindowFocusEvent {
    bool gained{false};
};

struct MouseMoveEvent {
    core::Point position;
    core::Point delta;
};

struct MouseButtonEvent {
    MouseButton button{MouseButton::Left};
    core::Point position;
    bool down{true};
    uint8_t clicks{1};
};

struct MouseWheelEvent {
    float xOffset{0.0f};
    float yOffset{0.0f}; // Positive is scroll up, negative is scroll down
    core::Point position;
};

struct KeyEvent {
    int32_t keyCode{0};
    uint32_t scanCode{0};
    KeyModifiers modifiers;
    bool down{true};
    bool repeat{false};
};

struct TextInputEvent {
    std::string text;
};

using EventData = std::variant<
    QuitEvent,
    WindowResizeEvent,
    WindowFocusEvent,
    MouseMoveEvent,
    MouseButtonEvent,
    MouseWheelEvent,
    KeyEvent,
    TextInputEvent
>;

struct Event {
    EventType type;
    EventData data;

    template <typename T>
    [[nodiscard]] const T* get() const noexcept {
        return std::get_if<T>(&data);
    }
};

} // namespace browser::platform

