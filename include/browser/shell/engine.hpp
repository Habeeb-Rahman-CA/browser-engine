#pragma once

#include "browser/platform/window.hpp"
#include "browser/events/event_dispatcher.hpp"
#include "browser/shell/frame_scheduler.hpp"
#include "browser/dom/document.hpp"
#include "browser/style/computed_style.hpp"
#include "browser/layout/layout_box.hpp"
#include "browser/graphics/display_list.hpp"
#include "browser/networking/resource_loader.hpp"
#include "browser/javascript/interpreter.hpp"
#include "browser/javascript/dom_bindings.hpp"

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace browser::shell {

struct EngineConfig {
    platform::WindowConfig window;
    std::string initialUrl{"about:blank"};
    bool dumpDiagnosticsOnExit{true};
    uint64_t maxFrames{0}; // 0 = run until quit
    bool verbose{false};
};

class BrowserEngine {
public:
    explicit BrowserEngine(EngineConfig config);
    ~BrowserEngine();

    BrowserEngine(const BrowserEngine&) = delete;
    BrowserEngine& operator=(const BrowserEngine&) = delete;
    BrowserEngine(BrowserEngine&&) noexcept = default;
    BrowserEngine& operator=(BrowserEngine&&) noexcept = default;

    // Initialization & lifecycle
    [[nodiscard]] bool initialize();
    int run();
    void step();
    void stop();

    // Navigation & Resource loading
    bool loadUrl(std::string_view urlOrPath);
    bool loadHtml(std::string_view htmlContent, std::string_view baseUrl = "about:blank");
    void goBack();
    void goForward();
    void reload();
    [[nodiscard]] bool canGoBack() const noexcept;
    [[nodiscard]] bool canGoForward() const noexcept;

    // Interaction & Scripting
    bool dispatchClick(core::Point position);
    std::string evaluateJavaScript(std::string_view code);
    bool saveScreenshot(std::string_view filename);

    // Scrolling & Viewport
    void scrollBy(int deltaY);
    [[nodiscard]] core::Point scrollOffset() const noexcept { return m_scrollOffset; }
    [[nodiscard]] core::Rect viewportRect() const noexcept;

    [[nodiscard]] bool isRunning() const noexcept { return m_running; }
    [[nodiscard]] const EngineConfig& config() const noexcept { return m_config; }
    [[nodiscard]] const std::string& currentUrl() const noexcept { return m_currentUrl; }

    // Subsystems access
    [[nodiscard]] platform::BrowserWindow& window() noexcept { return m_window; }
    [[nodiscard]] const platform::BrowserWindow& window() const noexcept { return m_window; }
    [[nodiscard]] events::EventDispatcher& eventDispatcher() noexcept { return m_dispatcher; }
    [[nodiscard]] FrameScheduler& scheduler() noexcept { return m_scheduler; }
    [[nodiscard]] const FrameScheduler& scheduler() const noexcept { return m_scheduler; }
    [[nodiscard]] std::shared_ptr<dom::Document> document() const noexcept { return m_document; }
    [[nodiscard]] const layout::LayoutBox* rootLayoutBox() const noexcept { return m_rootLayoutBox.get(); }
    [[nodiscard]] const graphics::DisplayList& displayList() const noexcept { return m_displayList; }
    [[nodiscard]] const std::unordered_map<const dom::Node*, std::shared_ptr<style::ComputedStyle>>& styleMap() const noexcept { return m_styleMap; }
    [[nodiscard]] javascript::Interpreter* jsInterpreter() noexcept { return m_jsInterpreter.get(); }
    [[nodiscard]] javascript::DomBindings* domBindings() noexcept { return m_domBindings.get(); }

private:
    void registerDefaultEventListeners();
    void relayout();
    void update(double dt);
    void render();
    void renderBrowserChrome();

    EngineConfig m_config;
    platform::BrowserWindow m_window;
    events::EventDispatcher m_dispatcher;
    FrameScheduler m_scheduler;
    networking::ResourceLoader m_resourceLoader;

    bool m_initialized{false};
    bool m_running{false};
    std::string m_currentUrl;

    // Navigation history
    std::vector<std::string> m_history;
    size_t m_historyIndex{0};

    // Document & Render pipeline state
    std::shared_ptr<dom::Document> m_document;
    std::unordered_map<const dom::Node*, std::shared_ptr<style::ComputedStyle>> m_styleMap;
    std::unique_ptr<layout::LayoutBox> m_rootLayoutBox;
    graphics::DisplayList m_displayList;

    // JavaScript runtime & host bindings
    std::unique_ptr<javascript::Interpreter> m_jsInterpreter;
    std::unique_ptr<javascript::DomBindings> m_domBindings;

    // Interaction & Viewport
    core::Point m_scrollOffset{0, 0};
    int m_maxScrollY{0};
    core::Point m_lastMousePos{0, 0};
    std::string m_statusMessage{"Ready"};
};

} // namespace browser::shell
