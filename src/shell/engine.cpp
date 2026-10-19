#include "browser/shell/engine.hpp"
#include "browser/core/diagnostics.hpp"
#include "browser/html/parser.hpp"
#include "browser/style/style_resolver.hpp"
#include "browser/layout/layout_tree_builder.hpp"
#include "browser/layout/layout_engine.hpp"
#include "browser/graphics/display_list_builder.hpp"
#include "browser/graphics/rasterizer.hpp"
#include "browser/css/css_parser.hpp"

#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace browser::shell {

namespace {

} // namespace

BrowserEngine::BrowserEngine(EngineConfig config)
    : m_config(std::move(config)),
      m_window(m_config.window),
      m_scheduler(60.0),
      m_currentUrl(m_config.initialUrl) {}

BrowserEngine::~BrowserEngine() {
    stop();
}

core::Rect BrowserEngine::viewportRect() const noexcept {
    auto winSize = m_window.size();
    const int toolbarHeight = 44;
    const int statusBarHeight = 24;
    int h = std::max(0, winSize.height - toolbarHeight - statusBarHeight);
    return core::Rect{0, toolbarHeight, winSize.width, h};
}

bool BrowserEngine::initialize() {
    if (m_initialized) {
        return true;
    }

    core::BrowserDiagnostics::instance().setVerbose(m_config.verbose);
    LOG_INFO("Engine", "Initializing BrowserEngine...");

    if (!m_window.initialize()) {
        LOG_ERROR("Engine", "Failed to initialize native window");
        return false;
    }

    registerDefaultEventListeners();
    m_initialized = true;

    // Load initial page
    loadUrl(m_currentUrl);

    LOG_INFO("Engine", "BrowserEngine initialized successfully");
    return true;
}

void BrowserEngine::registerDefaultEventListeners() {
    // Application quit
    m_dispatcher.subscribe(platform::EventType::Quit, [this](const platform::Event&) {
        LOG_INFO("Engine", "Received Quit event, shutting down...");
        m_running = false;
    });

    // Window resize
    m_dispatcher.subscribe(platform::EventType::WindowResize, [this](const platform::Event& ev) {
        if (const auto* resize = ev.get<platform::WindowResizeEvent>()) {
            LOG_INFO("Engine", "Window resized to " + std::to_string(resize->newSize.width) +
                               "x" + std::to_string(resize->newSize.height));
            relayout();
        }
    });

    // Key presses
    m_dispatcher.subscribe(platform::EventType::KeyDown, [this](const platform::Event& ev) {
        if (const auto* key = ev.get<platform::KeyEvent>()) {
            if (key->keyCode == 27 || key->scanCode == 41) { // ESC
                LOG_INFO("Engine", "Escape pressed, exiting...");
                m_running = false;
            } else if (key->keyCode == 'd' || key->keyCode == 'D' || key->scanCode == 7) {
                LOG_INFO("Engine", "D key pressed: Dumping diagnostics");
                core::BrowserDiagnostics::instance().dumpSummary(std::cout);
            } else if (key->keyCode == 'r' || key->keyCode == 'R') {
                reload();
            }
        }
    });

    // Vertical scrolling with mouse wheel
    m_dispatcher.subscribe(platform::EventType::MouseWheel, [this](const platform::Event& ev) {
        if (const auto* wheel = ev.get<platform::MouseWheelEvent>()) {
            scrollBy(static_cast<int>(-wheel->yOffset * 35.0f));
        }
    });

    // Mouse movement & link hover hit testing
    m_dispatcher.subscribe(platform::EventType::MouseMove, [this](const platform::Event& ev) {
        if (const auto* mouse = ev.get<platform::MouseMoveEvent>()) {
            m_lastMousePos = mouse->position;
            auto vp = viewportRect();
            if (m_rootLayoutBox && vp.contains(mouse->position)) {
                if (const auto* elem = graphics::Rasterizer::hitTest(*m_rootLayoutBox, mouse->position, vp, m_scrollOffset)) {
                    // Check if element or ancestor is a link
                    std::string href;
                    const dom::Node* curr = elem;
                    while (curr) {
                        if (curr->nodeType() == dom::NodeType::Element) {
                            const auto* el = static_cast<const dom::Element*>(curr);
                            if (el->tagName() == "a" && el->hasAttribute("href")) {
                                href = el->getAttribute("href");
                                break;
                            }
                        }
                        curr = curr->parentNode().get();
                    }
                    if (!href.empty()) {
                        m_statusMessage = "Link: " + href;
                        return;
                    }
                }
            }
            m_statusMessage = "Ready";
        }
    });

    // Mouse click handling for Chrome navigation buttons & hyperlinks
    m_dispatcher.subscribe(platform::EventType::MouseButtonDown, [this](const platform::Event& ev) {
        if (const auto* click = ev.get<platform::MouseButtonEvent>()) {
            if (click->button != platform::MouseButton::Left) return;

            int px = click->position.x;
            int py = click->position.y;

            // 1. Check Chrome toolbar buttons (y in [9, 35])
            if (py >= 9 && py <= 35) {
                // Back button (12 to 38)
                if (px >= 12 && px <= 38) {
                    goBack();
                    return;
                }
                // Forward button (44 to 70)
                if (px >= 44 && px <= 70) {
                    goForward();
                    return;
                }
                // Reload button (76 to 102)
                if (px >= 76 && px <= 102) {
                    reload();
                    return;
                }
            }

            // 2. Check Viewport content clicks (Hyperlink navigation & DOM event listeners)
            dispatchClick(click->position);
        }
    });
}

bool BrowserEngine::dispatchClick(core::Point position) {
    auto vp = viewportRect();
    if (m_rootLayoutBox && vp.contains(position)) {
        if (const auto* elem = graphics::Rasterizer::hitTest(*m_rootLayoutBox, position, vp, m_scrollOffset)) {
            // Dispatch click event to JavaScript DOM bindings
            if (m_domBindings) {
                auto targetNode = const_cast<dom::Element*>(elem)->shared_from_this();
                m_domBindings->dispatchEvent(targetNode, "click");
                if (m_domBindings->hasMutations()) {
                    m_domBindings->clearMutations();
                    // Recompute style and layout dynamically
                    style::StyleResolver resolver;
                    resolver.loadStylesFromDocument(*m_document);
                    m_styleMap.clear();
                    resolver.resolveTree(m_document, m_styleMap);
                    relayout();
                }
            }

            // Check hyperlink navigation
            const dom::Node* curr = elem;
            while (curr) {
                if (curr->nodeType() == dom::NodeType::Element) {
                    const auto* el = static_cast<const dom::Element*>(curr);
                    if (el->tagName() == "a" && el->hasAttribute("href")) {
                        std::string href = el->getAttribute("href");
                        LOG_INFO("Engine", "Clicked hyperlink: " + href);
                        loadUrl(href);
                        return true;
                    }
                }
                curr = curr->parentNode().get();
            }
            return true;
        }
    }
    return false;
}

std::string BrowserEngine::evaluateJavaScript(std::string_view code) {
    if (!m_jsInterpreter) {
        m_jsInterpreter = std::make_unique<javascript::Interpreter>();
    }
    if (!m_domBindings && m_document) {
        m_domBindings = std::make_unique<javascript::DomBindings>(*m_jsInterpreter, m_document);
    }
    auto val = m_jsInterpreter->evaluate(code);
    if (m_domBindings && m_domBindings->hasMutations()) {
        m_domBindings->clearMutations();
        style::StyleResolver resolver;
        resolver.loadStylesFromDocument(*m_document);
        m_styleMap.clear();
        resolver.resolveTree(m_document, m_styleMap);
        relayout();
    }
    return val.toString();
}

bool BrowserEngine::saveScreenshot(std::string_view filename) {
    if (!m_window.sdlRenderer()) return false;
    SDL_Surface* surface = SDL_RenderReadPixels(m_window.sdlRenderer(), nullptr);
    if (!surface) return false;
    bool success = SDL_SaveBMP(surface, std::string(filename).c_str());
    SDL_DestroySurface(surface);
    return success;
}

bool BrowserEngine::loadHtml(std::string_view htmlContent, std::string_view baseUrl) {
    auto targetUrl = networking::Url(std::string(baseUrl));
    if (!targetUrl.isValid()) {
        targetUrl = networking::Url("about:blank");
    }

    // Parse HTML into DOM
    m_document = html::HtmlParser::parseDocument(htmlContent);

    // Initialize JavaScript runtime & DOM host bindings
    m_jsInterpreter = std::make_unique<javascript::Interpreter>();
    m_domBindings = std::make_unique<javascript::DomBindings>(*m_jsInterpreter, m_document);

    // Execute scripts in document
    auto scriptElements = m_document->getElementsByTagName("script");
    for (const auto& scriptEl : scriptElements) {
        if (scriptEl->hasAttribute("src")) {
            std::string src = scriptEl->getAttribute("src");
            auto scriptUrl = networking::Url::resolve(targetUrl, src);
            auto scriptRes = m_resourceLoader.load(scriptUrl, networking::ResourceType::Script);
            if (scriptRes.isSuccess()) {
                m_jsInterpreter->evaluate(scriptRes.asText());
            }
        } else {
            std::string code = scriptEl->textContent();
            if (!code.empty()) {
                m_jsInterpreter->evaluate(code);
            }
        }
    }

    if (m_domBindings->hasMutations()) {
        m_domBindings->clearMutations();
    }

    // Resolve CSS styles
    style::StyleResolver resolver;
    resolver.loadStylesFromDocument(*m_document);

    // Also fetch external stylesheets linked via <link rel="stylesheet" href="...">
    auto linkElements = m_document->getElementsByTagName("link");
    for (const auto& linkEl : linkElements) {
        if (linkEl->getAttribute("rel") == "stylesheet" && linkEl->hasAttribute("href")) {
            std::string href = linkEl->getAttribute("href");
            auto cssUrl = networking::Url::resolve(targetUrl, href);
            auto cssResource = m_resourceLoader.load(cssUrl, networking::ResourceType::Css);
            if (cssResource.isSuccess()) {
                resolver.addAuthorStyleSheet(css::CssParser::parse(cssResource.asText()));
            }
        }
    }

    m_styleMap.clear();
    resolver.resolveTree(m_document, m_styleMap);

    // Build and layout render tree
    relayout();

    // Reset scroll offset
    m_scrollOffset = {0, 0};
    m_currentUrl = std::string(baseUrl);

    std::string pageTitle = m_document->title();
    if (!pageTitle.empty()) {
        m_window.setTitle("Browser Engine - " + pageTitle);
    } else {
        m_window.setTitle("Browser Engine - " + m_currentUrl);
    }

    return true;
}

bool BrowserEngine::loadUrl(std::string_view urlOrPath) {
    std::string target(urlOrPath);
    LOG_INFO("Engine", "Navigating to: " + target);

    std::string htmlContent;
    networking::Url targetUrl(target);
    if (!targetUrl.isValid() && !target.starts_with("about:") && !target.starts_with("http://") && !target.starts_with("https://") && !target.starts_with("file://")) {
        std::error_code ec;
        auto absPath = std::filesystem::absolute(std::filesystem::path(target), ec);
        targetUrl = networking::Url("file://" + (ec ? target : absPath.string()));
    }

    if (target == "about:blank" || target.empty()) {
        htmlContent =
            "<!DOCTYPE html><html><head><title>New Tab</title>"
            "<style>"
            "body { font-family: ui-sans-serif, system-ui, sans-serif; background-color: #faf8f5; color: #48433e; margin: 0; padding: 40px; }"
            ".card { width: 560px; margin: 20px auto; background-color: #ffffff; border: 1px solid #e8e3d9; border-radius: 20px; padding: 36px; }"
            ".tag { display: inline-block; background-color: #edf2f9; color: #6a7fa8; border-radius: 12px; padding: 4px 12px; font-size: 13px; font-weight: bold; margin-bottom: 12px; }"
            "h1 { color: #3d3934; font-size: 26px; margin: 8px 0; }"
            "p.desc { color: #7c756c; font-size: 15px; line-height: 1.6; margin-bottom: 24px; }"
            ".nav-item { display: block; background-color: #f6f3ec; border: 1px solid #e5dfd4; border-radius: 14px; padding: 12px 18px; margin: 10px 0; color: #48433e; text-decoration: none; }"
            ".footer { color: #a49e94; font-size: 12px; margin-top: 24px; text-align: center; }"
            "</style></head>"
            "<body><div class=\"card\">"
            "<span class=\"tag\">Browser Engine</span>"
            "<h1>Welcome to your new tab</h1>"
            "<p class=\"desc\">A minimalist, lightweight browser engine built from scratch in modern C++20.</p>"
            "<div class=\"nav-item\"><strong>Quick Demonstrations</strong></div>"
            "<p><a href=\"examples/basic/index.html\">Basic HTML &amp; Layout Demo</a></p>"
            "<p><a href=\"examples/css/index.html\">CSS Box Model &amp; Styling Showcase</a></p>"
            "<p><a href=\"examples/scripts/index.html\">Interactive JavaScript Counter</a></p>"
            "<div class=\"footer\">Press Escape to exit | D for Diagnostics</div>"
            "</div></body></html>";
    } else {
        auto resource = m_resourceLoader.load(targetUrl, networking::ResourceType::Html);
        if (resource.isSuccess()) {
            htmlContent = resource.asText();
        } else {
            // Error page
            htmlContent =
                "<!DOCTYPE html><html><head><title>Load Error</title>"
                "<style>"
                "body { font-family: ui-sans-serif, system-ui, sans-serif; background-color: #faf8f5; padding: 40px; color: #48433e; }"
                ".card { width: 520px; margin: 40px auto; background-color: #ffffff; border: 1px solid #f0ded9; border-radius: 20px; padding: 36px; }"
                ".tag { display: inline-block; background-color: #fbeee9; color: #b8624e; border-radius: 12px; padding: 4px 12px; font-size: 13px; font-weight: bold; margin-bottom: 12px; }"
                "h1 { color: #a84c37; font-size: 24px; margin: 8px 0; }"
                "p { color: #7c756c; font-size: 15px; line-height: 1.5; }"
                "</style></head>"
                "<body><div class=\"card\">"
                "<span class=\"tag\">Network / File Error</span>"
                "<h1>Unable to load page</h1>"
                "<p>Failed to load resource: <strong>" + target + "</strong></p>"
                "<p>Status: " + resource.errorMessage + "</p>"
                "</div></body></html>";
        }
    }

    bool loaded = loadHtml(htmlContent, targetUrl.isValid() ? targetUrl.toString() : target);
    m_currentUrl = target;

    // Update history stack
    if (m_history.empty() || m_history[m_historyIndex] != target) {
        if (!m_history.empty() && m_historyIndex + 1 < m_history.size()) {
            m_history.erase(m_history.begin() + static_cast<ptrdiff_t>(m_historyIndex + 1), m_history.end());
        }
        m_history.push_back(target);
        m_historyIndex = m_history.size() - 1;
    }

    return loaded;
}

void BrowserEngine::relayout() {
    if (!m_document) return;

    auto vp = viewportRect();
    core::Size vpSize{vp.width, vp.height};

    m_rootLayoutBox = layout::LayoutTreeBuilder::build(m_document, m_styleMap);
    if (m_rootLayoutBox) {
        layout::LayoutEngine::layout(*m_rootLayoutBox, vpSize);
        m_displayList = graphics::DisplayListBuilder::build(*m_rootLayoutBox);

        float contentH = m_rootLayoutBox->dimensions().marginBox().height;
        m_maxScrollY = std::max(0, static_cast<int>(contentH) - vp.height);
    }
}

void BrowserEngine::scrollBy(int deltaY) {
    m_scrollOffset.y = std::clamp(m_scrollOffset.y + deltaY, 0, m_maxScrollY);
}

void BrowserEngine::goBack() {
    if (canGoBack()) {
        m_historyIndex--;
        loadUrl(m_history[m_historyIndex]);
    }
}

void BrowserEngine::goForward() {
    if (canGoForward()) {
        m_historyIndex++;
        loadUrl(m_history[m_historyIndex]);
    }
}

void BrowserEngine::reload() {
    if (!m_currentUrl.empty()) {
        loadUrl(m_currentUrl);
    }
}

bool BrowserEngine::canGoBack() const noexcept {
    return m_historyIndex > 0;
}

bool BrowserEngine::canGoForward() const noexcept {
    return !m_history.empty() && m_historyIndex + 1 < m_history.size();
}

void BrowserEngine::step() {
    m_scheduler.beginFrame();

    m_window.pollEvents(m_dispatcher);

    update(m_scheduler.deltaTime());

    render();

    m_scheduler.endFrame();
}

int BrowserEngine::run() {
    if (!m_initialized && !initialize()) {
        return 1;
    }

    m_running = true;
    m_scheduler.reset();

    LOG_INFO("Engine", "Entering main event loop (target FPS: " +
                       std::to_string(m_scheduler.targetFps()) + ")...");

    while (m_running && m_window.isOpen()) {
        step();

        if (m_config.maxFrames > 0 && m_scheduler.frameCount() >= m_config.maxFrames) {
            LOG_INFO("Engine", "Reached maximum frame limit (" +
                               std::to_string(m_config.maxFrames) + "), terminating loop.");
            break;
        }
    }

    LOG_INFO("Engine", "Exited event loop. Total frames rendered: " +
                       std::to_string(m_scheduler.frameCount()));

    if (m_config.dumpDiagnosticsOnExit) {
        core::BrowserDiagnostics::instance().dumpSummary(std::cout);
    }

    m_window.close();
    m_running = false;
    return 0;
}

void BrowserEngine::stop() {
    if (m_running) {
        m_running = false;
        m_window.close();
    }
}

void BrowserEngine::update(double /*dt*/) {}

void BrowserEngine::render() {
    core::ScopedTimer timer(core::Subsystem::Painting);

    // Clear window buffer with soft warm cream paper tone
    m_window.clear(core::Color::fromRgb(250, 248, 245));

    // 1. Rasterize Web Content Viewport
    auto vp = viewportRect();
    if (!m_displayList.empty() && m_window.sdlRenderer()) {
        graphics::Rasterizer::rasterize(m_window.sdlRenderer(), m_displayList, vp, m_scrollOffset);
    }

    // 2. Render Vertical Scrollbar if content overflows
    if (m_maxScrollY > 0 && vp.height > 40) {
        const int barWidth = 8;
        int trackHeight = vp.height;
        float ratio = static_cast<float>(vp.height) / static_cast<float>(vp.height + m_maxScrollY);
        int thumbHeight = std::max(24, static_cast<int>(static_cast<float>(trackHeight) * ratio));
        float scrollRatio = static_cast<float>(m_scrollOffset.y) / static_cast<float>(m_maxScrollY);
        int thumbY = vp.y + static_cast<int>(static_cast<float>(trackHeight - thumbHeight) * scrollRatio);

        // Soft rounded pill scrollbar thumb
        m_window.drawRoundedRect(
            core::Rect{vp.width - barWidth - 4, thumbY, barWidth, thumbHeight},
            4.0f,
            core::Color::fromRgba(185, 178, 168, 170),
            true
        );
    }

    // 3. Render Browser Chrome UI (Toolbar, URL Bar, Buttons, Status Bar)
    renderBrowserChrome();

    m_window.present();
}

void BrowserEngine::renderBrowserChrome() {
    auto winSize = m_window.size();
    if (winSize.isEmpty()) return;

    const int toolbarHeight = 46;
    const int statusBarHeight = 26;

    // 1. Top Navigation Toolbar Background (clean warm off-white)
    m_window.drawRect(
        core::Rect{0, 0, winSize.width, toolbarHeight},
        core::Color::fromRgb(246, 244, 240),
        true
    );
    // Subtle low-contrast bottom border
    m_window.drawLine(
        core::Point{0, toolbarHeight},
        core::Point{winSize.width, toolbarHeight},
        core::Color::fromRgb(232, 228, 220)
    );

    // 2. Navigation Buttons (Back, Forward, Reload) with rounded pill shapes
    const int btnSize = 28;
    const int btnY = 9;

    // Back button
    core::Color backColor = canGoBack() ? core::Color::fromRgb(108, 102, 94) : core::Color::fromRgb(196, 191, 183);
    m_window.drawRoundedRect(core::Rect{12, btnY, btnSize, btnSize}, 8.0f, core::Color::fromRgb(239, 236, 230), true);
    m_window.drawRoundedRect(core::Rect{12, btnY, btnSize, btnSize}, 8.0f, core::Color::fromRgb(226, 221, 212), false);
    // Draw '<' icon
    m_window.drawLine(core::Point{28, 17}, core::Point{22, 23}, backColor);
    m_window.drawLine(core::Point{22, 23}, core::Point{28, 29}, backColor);

    // Forward button
    core::Color fwdColor = canGoForward() ? core::Color::fromRgb(108, 102, 94) : core::Color::fromRgb(196, 191, 183);
    m_window.drawRoundedRect(core::Rect{46, btnY, btnSize, btnSize}, 8.0f, core::Color::fromRgb(239, 236, 230), true);
    m_window.drawRoundedRect(core::Rect{46, btnY, btnSize, btnSize}, 8.0f, core::Color::fromRgb(226, 221, 212), false);
    // Draw '>' icon
    m_window.drawLine(core::Point{58, 17}, core::Point{64, 23}, fwdColor);
    m_window.drawLine(core::Point{64, 23}, core::Point{58, 29}, fwdColor);

    // Reload button
    m_window.drawRoundedRect(core::Rect{80, btnY, btnSize, btnSize}, 8.0f, core::Color::fromRgb(239, 236, 230), true);
    m_window.drawRoundedRect(core::Rect{80, btnY, btnSize, btnSize}, 8.0f, core::Color::fromRgb(226, 221, 212), false);
    m_window.drawRoundedRect(core::Rect{90, 19, 8, 8}, 2.0f, core::Color::fromRgb(108, 102, 94), false);

    // 3. Address Bar Capsule (Fully rounded pill with radius 15px)
    int addressBarX = 120;
    int addressBarWidth = winSize.width - addressBarX - 16;
    if (addressBarWidth > 60) {
        m_window.drawRoundedRect(core::Rect{addressBarX, 7, addressBarWidth, 31}, 15.0f, core::Color::white(), true);
        m_window.drawRoundedRect(core::Rect{addressBarX, 7, addressBarWidth, 31}, 15.0f, core::Color::fromRgb(228, 223, 214), false);

        // Cute soft pastel badge on left (soft sage pill)
        m_window.drawRoundedRect(core::Rect{addressBarX + 7, 12, 21, 21}, 10.5f, core::Color::fromRgb(228, 236, 230), true);
        m_window.drawRoundedRect(core::Rect{addressBarX + 14, 19, 7, 7}, 3.5f, core::Color::fromRgb(118, 168, 128), true);

        // Render URL text in address bar
        if (m_window.sdlRenderer() && !m_currentUrl.empty()) {
            SDL_SetRenderDrawColor(m_window.sdlRenderer(), 74, 69, 62, 255);
            SDL_RenderDebugText(m_window.sdlRenderer(), static_cast<float>(addressBarX + 36), 15.0f, m_currentUrl.c_str());
        }
    }

    // 4. Bottom Status Bar (minimalist soft footer)
    int statusY = winSize.height - statusBarHeight;
    m_window.drawRect(core::Rect{0, statusY, winSize.width, statusBarHeight}, core::Color::fromRgb(246, 244, 240), true);
    m_window.drawLine(core::Point{0, statusY}, core::Point{winSize.width, statusY}, core::Color::fromRgb(232, 228, 220));

    if (m_window.sdlRenderer()) {
        SDL_SetRenderDrawColor(m_window.sdlRenderer(), 138, 132, 122, 255);
        SDL_RenderDebugText(m_window.sdlRenderer(), 16.0f, static_cast<float>(statusY + 7), m_statusMessage.c_str());

        // FPS indicator on right in soft muted style
        std::string fpsStr = "FPS: " + std::to_string(static_cast<int>(m_scheduler.currentFps()));
        SDL_RenderDebugText(m_window.sdlRenderer(), static_cast<float>(winSize.width - 96), static_cast<float>(statusY + 7), fpsStr.c_str());
    }
}

} // namespace browser::shell
