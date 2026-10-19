#include "browser/platform/window.hpp"
#include "browser/core/diagnostics.hpp"

#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace browser::platform {

namespace {

MouseButton translateMouseButton(uint8_t sdlButton) {
    switch (sdlButton) {
        case SDL_BUTTON_LEFT:   return MouseButton::Left;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        default:                return MouseButton::Other;
    }
}

KeyModifiers translateModifiers(SDL_Keymod mod) {
    KeyModifiers km;
    km.shift = (mod & SDL_KMOD_SHIFT) != 0;
    km.ctrl  = (mod & SDL_KMOD_CTRL) != 0;
    km.alt   = (mod & SDL_KMOD_ALT) != 0;
    km.super = (mod & SDL_KMOD_GUI) != 0;
    return km;
}

} // namespace

BrowserWindow::BrowserWindow(WindowConfig config)
    : m_config(std::move(config)),
      m_size{m_config.width, m_config.height} {}

BrowserWindow::~BrowserWindow() {
    close();
}

BrowserWindow::BrowserWindow(BrowserWindow&& other) noexcept
    : m_config(std::move(other.m_config)),
      m_size(other.m_size),
      m_scaleFactor(other.m_scaleFactor),
      m_isOpen(other.m_isOpen),
      m_sdlInitialized(other.m_sdlInitialized),
      m_window(other.m_window),
      m_renderer(other.m_renderer) {
    other.m_window = nullptr;
    other.m_renderer = nullptr;
    other.m_isOpen = false;
    other.m_sdlInitialized = false;
}

BrowserWindow& BrowserWindow::operator=(BrowserWindow&& other) noexcept {
    if (this != &other) {
        close();
        m_config = std::move(other.m_config);
        m_size = other.m_size;
        m_scaleFactor = other.m_scaleFactor;
        m_isOpen = other.m_isOpen;
        m_sdlInitialized = other.m_sdlInitialized;
        m_window = other.m_window;
        m_renderer = other.m_renderer;

        other.m_window = nullptr;
        other.m_renderer = nullptr;
        other.m_isOpen = false;
        other.m_sdlInitialized = false;
    }
    return *this;
}

bool BrowserWindow::initialize() {
    if (m_isOpen) {
        return true;
    }

    if (m_config.headless) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    }

    // Try normal SDL initialization
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        // If native video failed (e.g. no X11/Wayland display server available in container), fallback to dummy
        LOG_WARN("Window", std::string("Standard video init failed: ") + SDL_GetError() + ". Trying dummy driver...");
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            LOG_ERROR("Window", std::string("Failed to initialize SDL: ") + SDL_GetError());
            return false;
        }
        m_config.headless = true;
    }
    m_sdlInitialized = true;

    SDL_WindowFlags flags = 0;
    if (m_config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    if (m_config.headless) {
        flags |= SDL_WINDOW_HIDDEN;
    }

    m_window = SDL_CreateWindow(
        m_config.title.c_str(),
        m_config.width,
        m_config.height,
        flags
    );

    if (!m_window) {
        LOG_ERROR("Window", std::string("Failed to create SDL window: ") + SDL_GetError());
        close();
        return false;
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);
    if (!m_renderer) {
        LOG_ERROR("Window", std::string("Failed to create SDL renderer: ") + SDL_GetError());
        close();
        return false;
    }

    // Query actual size and scale factor
    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);
    m_size = {w, h};
    m_scaleFactor = SDL_GetWindowDisplayScale(m_window);

    m_isOpen = true;
    LOG_INFO("Window", std::string("Window created (") + std::to_string(w) + "x" + std::to_string(h) +
             ", scale=" + std::to_string(m_scaleFactor) +
             ", driver=" + (SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none") + ")");

    return true;
}

void BrowserWindow::close() {
    if (m_renderer) {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_sdlInitialized) {
        SDL_Quit();
        m_sdlInitialized = false;
    }
    m_isOpen = false;
}

void BrowserWindow::setTitle(std::string_view title) {
    m_config.title = std::string(title);
    if (m_window) {
        SDL_SetWindowTitle(m_window, m_config.title.c_str());
    }
}

void BrowserWindow::pollEvents(events::EventDispatcher& dispatcher) {
    if (!m_isOpen) return;

    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        processSdlEvent(&sdlEvent, dispatcher);
    }
}

void BrowserWindow::processSdlEvent(const void* sdlEventPtr, events::EventDispatcher& dispatcher) {
    const auto& sdlEvent = *static_cast<const SDL_Event*>(sdlEventPtr);

    switch (sdlEvent.type) {
        case SDL_EVENT_QUIT: {
            Event ev{EventType::Quit, QuitEvent{}};
            dispatcher.dispatch(ev);
            m_isOpen = false;
            break;
        }

        case SDL_EVENT_WINDOW_RESIZED: {
            m_size = {sdlEvent.window.data1, sdlEvent.window.data2};
            m_scaleFactor = SDL_GetWindowDisplayScale(m_window);
            Event ev{EventType::WindowResize, WindowResizeEvent{m_size}};
            dispatcher.dispatch(ev);
            break;
        }

        case SDL_EVENT_WINDOW_FOCUS_GAINED: {
            Event ev{EventType::WindowFocus, WindowFocusEvent{true}};
            dispatcher.dispatch(ev);
            break;
        }

        case SDL_EVENT_WINDOW_FOCUS_LOST: {
            Event ev{EventType::WindowFocus, WindowFocusEvent{false}};
            dispatcher.dispatch(ev);
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            Event ev{
                EventType::MouseMove,
                MouseMoveEvent{
                    core::Point{static_cast<int>(sdlEvent.motion.x), static_cast<int>(sdlEvent.motion.y)},
                    core::Point{static_cast<int>(sdlEvent.motion.xrel), static_cast<int>(sdlEvent.motion.yrel)}
                }
            };
            dispatcher.dispatch(ev);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            bool down = (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            Event ev{
                down ? EventType::MouseButtonDown : EventType::MouseButtonUp,
                MouseButtonEvent{
                    translateMouseButton(sdlEvent.button.button),
                    core::Point{static_cast<int>(sdlEvent.button.x), static_cast<int>(sdlEvent.button.y)},
                    down,
                    sdlEvent.button.clicks
                }
            };
            dispatcher.dispatch(ev);
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL: {
            Event ev{
                EventType::MouseWheel,
                MouseWheelEvent{
                    sdlEvent.wheel.x,
                    sdlEvent.wheel.y,
                    core::Point{static_cast<int>(sdlEvent.wheel.mouse_x), static_cast<int>(sdlEvent.wheel.mouse_y)}
                }
            };
            dispatcher.dispatch(ev);
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            bool down = (sdlEvent.type == SDL_EVENT_KEY_DOWN);
            Event ev{
                down ? EventType::KeyDown : EventType::KeyUp,
                KeyEvent{
                    static_cast<int32_t>(sdlEvent.key.key),
                    static_cast<uint32_t>(sdlEvent.key.scancode),
                    translateModifiers(sdlEvent.key.mod),
                    down,
                    sdlEvent.key.repeat
                }
            };
            dispatcher.dispatch(ev);
            break;
        }

        case SDL_EVENT_TEXT_INPUT: {
            Event ev{
                EventType::TextInput,
                TextInputEvent{sdlEvent.text.text ? sdlEvent.text.text : ""}
            };
            dispatcher.dispatch(ev);
            break;
        }

        default:
            break;
    }
}

void BrowserWindow::clear(const core::Color& color) {
    if (!m_renderer) return;
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(m_renderer);
}

void BrowserWindow::present() {
    if (!m_renderer) return;
    SDL_RenderPresent(m_renderer);
}

void BrowserWindow::drawRect(const core::Rect& rect, const core::Color& color, bool filled) {
    if (!m_renderer) return;
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_FRect frect{
        static_cast<float>(rect.x),
        static_cast<float>(rect.y),
        static_cast<float>(rect.width),
        static_cast<float>(rect.height)
    };
    if (filled) {
        SDL_RenderFillRect(m_renderer, &frect);
    } else {
        SDL_RenderRect(m_renderer, &frect);
    }
}

void BrowserWindow::drawRoundedRect(const core::Rect& rect, float radius, const core::Color& color, bool filled) {
    if (!m_renderer) return;

    radius = std::clamp(radius, 0.0f, std::min(static_cast<float>(rect.width), static_cast<float>(rect.height)) / 2.0f);
    if (radius <= 0.0f) {
        drawRect(rect, color, filled);
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    float x = static_cast<float>(rect.x);
    float y = static_cast<float>(rect.y);
    float w = static_cast<float>(rect.width);
    float h = static_cast<float>(rect.height);
    float r = radius;

    if (filled) {
        SDL_FRect center{x + r, y, w - 2.0f * r, h};
        SDL_FRect left{x, y + r, r, h - 2.0f * r};
        SDL_FRect right{x + w - r, y + r, r, h - 2.0f * r};
        SDL_RenderFillRect(m_renderer, &center);
        SDL_RenderFillRect(m_renderer, &left);
        SDL_RenderFillRect(m_renderer, &right);

        int ir = static_cast<int>(r);
        for (int dy = 0; dy <= ir; ++dy) {
            float dx = std::sqrt(std::max(0.0f, r * r - static_cast<float>(dy * dy)));
            SDL_RenderLine(m_renderer, x + r - dx, y + r - static_cast<float>(dy), x + r, y + r - static_cast<float>(dy));
            SDL_RenderLine(m_renderer, x + w - r, y + r - static_cast<float>(dy), x + w - r + dx, y + r - static_cast<float>(dy));
            SDL_RenderLine(m_renderer, x + r - dx, y + h - r + static_cast<float>(dy), x + r, y + h - r + static_cast<float>(dy));
            SDL_RenderLine(m_renderer, x + w - r, y + h - r + static_cast<float>(dy), x + w - r + dx, y + h - r + static_cast<float>(dy));
        }
    } else {
        SDL_RenderLine(m_renderer, x + r, y, x + w - r, y);
        SDL_RenderLine(m_renderer, x + r, y + h, x + w - r, y + h);
        SDL_RenderLine(m_renderer, x, y + r, x, y + h - r);
        SDL_RenderLine(m_renderer, x + w, y + r, x + w, y + h - r);

        int steps = std::max(6, static_cast<int>(r * 1.5f));
        float stepsF = static_cast<float>(steps);
        for (int i = 0; i < steps; ++i) {
            float a1 = (static_cast<float>(i) / stepsF) * (3.14159265f / 2.0f);
            float a2 = (static_cast<float>(i + 1) / stepsF) * (3.14159265f / 2.0f);
            float cos1 = std::cos(a1) * r, sin1 = std::sin(a1) * r;
            float cos2 = std::cos(a2) * r, sin2 = std::sin(a2) * r;

            SDL_RenderLine(m_renderer, x + r - cos1, y + r - sin1, x + r - cos2, y + r - sin2);
            SDL_RenderLine(m_renderer, x + w - r + cos1, y + r - sin1, x + w - r + cos2, y + r - sin2);
            SDL_RenderLine(m_renderer, x + r - cos1, y + h - r + sin1, x + r - cos2, y + h - r + sin2);
            SDL_RenderLine(m_renderer, x + w - r + cos1, y + h - r + sin1, x + w - r + cos2, y + h - r + sin2);
        }
    }
}

void BrowserWindow::drawLine(const core::Point& p1, const core::Point& p2, const core::Color& color) {
    if (!m_renderer) return;
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderLine(m_renderer,
                   static_cast<float>(p1.x), static_cast<float>(p1.y),
                   static_cast<float>(p2.x), static_cast<float>(p2.y));
}

} // namespace browser::platform

