#pragma once

#include "browser/core/types.hpp"
#include "browser/platform/event.hpp"
#include "browser/events/event_dispatcher.hpp"

#include <string>
#include <memory>

struct SDL_Window;
struct SDL_Renderer;

namespace browser::platform {

struct WindowConfig {
    std::string title{"Browser Engine"};
    int width{1024};
    int height{768};
    bool resizable{true};
    bool headless{false};
};

class BrowserWindow {
public:
    explicit BrowserWindow(WindowConfig config);
    ~BrowserWindow();

    BrowserWindow(const BrowserWindow&) = delete;
    BrowserWindow& operator=(const BrowserWindow&) = delete;
    BrowserWindow(BrowserWindow&&) noexcept;
    BrowserWindow& operator=(BrowserWindow&&) noexcept;

    // Window lifecycle
    [[nodiscard]] bool initialize();
    void close();
    [[nodiscard]] bool isOpen() const noexcept { return m_isOpen; }
    [[nodiscard]] bool isHeadless() const noexcept { return m_config.headless; }

    // Dimensions
    [[nodiscard]] core::Size size() const noexcept { return m_size; }
    [[nodiscard]] float scaleFactor() const noexcept { return m_scaleFactor; }
    void setTitle(std::string_view title);

    // Event processing
    void pollEvents(events::EventDispatcher& dispatcher);

    // Frame presentation
    void clear(const core::Color& color = core::Color::lightGray());
    void present();

    // Drawing primitives (used for diagnostics, chrome, and initial testing)
    void drawRect(const core::Rect& rect, const core::Color& color, bool filled = true);
    void drawRoundedRect(const core::Rect& rect, float radius, const core::Color& color, bool filled = true);
    void drawLine(const core::Point& p1, const core::Point& p2, const core::Color& color);

    // Accessors for graphics backend
    [[nodiscard]] SDL_Window* sdlWindow() const noexcept { return m_window; }
    [[nodiscard]] SDL_Renderer* sdlRenderer() const noexcept { return m_renderer; }

private:
    void processSdlEvent(const void* sdlEventPtr, events::EventDispatcher& dispatcher);

    WindowConfig m_config;
    core::Size m_size{0, 0};
    float m_scaleFactor{1.0f};
    bool m_isOpen{false};
    bool m_sdlInitialized{false};

    SDL_Window* m_window{nullptr};
    SDL_Renderer* m_renderer{nullptr};
};

} // namespace browser::platform

