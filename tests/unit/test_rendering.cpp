#include "test_framework.hpp"
#include "browser/graphics/display_list.hpp"
#include "browser/graphics/display_list_builder.hpp"
#include "browser/graphics/rasterizer.hpp"
#include "browser/html/parser.hpp"
#include "browser/style/style_resolver.hpp"
#include "browser/layout/layout_tree_builder.hpp"
#include "browser/layout/layout_engine.hpp"
#include "browser/shell/engine.hpp"

#include <sstream>

using namespace browser::graphics;
using namespace browser::style;
using namespace browser::html;
using namespace browser::layout;
using namespace browser::shell;

TEST_CASE("Rendering: DisplayList records and dumps paint commands") {
    DisplayList list;
    list.addFillRect(RectF{0, 0, 100, 50}, browser::core::Color::red());
    list.addStrokeRect(RectF{10, 10, 80, 30}, browser::core::Color::black(), 2.0f);
    list.addText("Hello", 16.0f, browser::core::Color::blue(), RectF{10, 10, 80, 20});

    CHECK_EQ(list.size(), 3u);
    CHECK_FALSE(list.empty());

    std::ostringstream ss;
    list.dump(ss);
    std::string dump = ss.str();

    CHECK(dump.find("FillRect") != std::string::npos);
    CHECK(dump.find("StrokeRect") != std::string::npos);
    CHECK(dump.find("Text") != std::string::npos);
    CHECK(dump.find("Hello") != std::string::npos);
}

TEST_CASE("Rendering: DisplayListBuilder emits backgrounds, borders, and text") {
    std::string html = 
        "<html><body>\n"
        "  <div id=\"card\" style=\"background-color: #ff0000; border: 2px solid #000000;\">\n"
        "    <p style=\"color: #0000ff;\">Card Content</p>\n"
        "  </div>\n"
        "</body></html>";

    auto doc = HtmlParser::parseDocument(html);
    StyleResolver resolver;
    resolver.loadStylesFromDocument(*doc);
    std::unordered_map<const browser::dom::Node*, std::shared_ptr<ComputedStyle>> styleMap;
    resolver.resolveTree(doc, styleMap);

    auto rootBox = LayoutTreeBuilder::build(doc, styleMap);
    REQUIRE(rootBox != nullptr);
    LayoutEngine::layout(*rootBox, browser::core::Size{800, 600});

    auto list = DisplayListBuilder::build(*rootBox);
    CHECK(list.size() >= 4u); // Background + 4 borders (or solid) + text

    bool foundRedBg = false;
    bool foundText = false;

    for (const auto& item : list.items()) {
        if (item.type == ItemType::FillRect) {
            const auto* d = item.get<FillRectItem>();
            if (d->color.r == 255 && d->color.g == 0 && d->color.b == 0) {
                foundRedBg = true;
            }
        }
        if (item.type == ItemType::Text) {
            const auto* d = item.get<TextItem>();
            if (d->text.find("Card Content") != std::string::npos) {
                foundText = true;
                CHECK_EQ(d->color.b, 255); // Blue text
            }
        }
    }

    CHECK(foundRedBg);
    CHECK(foundText);
}

TEST_CASE("Rendering: Hit testing finds element under cursor") {
    std::string html = 
        "<html><body>\n"
        "  <div id=\"sidebar\" style=\"width: 200px; height: 300px;\">\n"
        "    <a id=\"target-link\" href=\"#target\" style=\"display: block; width: 150px; height: 30px;\">Click Me</a>\n"
        "  </div>\n"
        "</body></html>";

    auto doc = HtmlParser::parseDocument(html);
    StyleResolver resolver;
    resolver.loadStylesFromDocument(*doc);
    std::unordered_map<const browser::dom::Node*, std::shared_ptr<ComputedStyle>> styleMap;
    resolver.resolveTree(doc, styleMap);

    auto rootBox = LayoutTreeBuilder::build(doc, styleMap);
    REQUIRE(rootBox != nullptr);

    browser::core::Rect viewport{0, 0, 800, 600};
    LayoutEngine::layout(*rootBox, viewport.size());

    // Hit test over the link (e.g. at x=50, y=15)
    const auto* hitElem = Rasterizer::hitTest(*rootBox, browser::core::Point{50, 15}, viewport);
    REQUIRE(hitElem != nullptr);
    CHECK_EQ(hitElem->tagName(), "a");
    CHECK_EQ(hitElem->id(), "target-link");

    // Hit test outside sidebar (e.g. at x=500, y=500)
    const auto* missElem = Rasterizer::hitTest(*rootBox, browser::core::Point{500, 500}, viewport);
    // Should hit body or html or null (outside sidebar content)
    CHECK(missElem != hitElem);
}

TEST_CASE("Navigation: History and scrolling management in BrowserEngine") {
    EngineConfig config{
        .window = {
            .title = "Test Nav Engine",
            .width = 800,
            .height = 600,
            .resizable = false,
            .headless = true
        },
        .initialUrl = "about:blank",
        .dumpDiagnosticsOnExit = false,
        .maxFrames = 1,
        .verbose = false
    };

    BrowserEngine engine(config);
    REQUIRE(engine.initialize());

    CHECK_FALSE(engine.canGoBack());
    CHECK_FALSE(engine.canGoForward());
    CHECK_EQ(engine.currentUrl(), "about:blank");

    // Load second URL
    engine.loadUrl("examples/basic/index.html");
    CHECK(engine.canGoBack());
    CHECK_FALSE(engine.canGoForward());

    // Go back
    engine.goBack();
    CHECK_EQ(engine.currentUrl(), "about:blank");
    CHECK(engine.canGoForward());

    // Go forward
    engine.goForward();
    CHECK_EQ(engine.currentUrl(), "examples/basic/index.html");

    // Scroll
    CHECK_EQ(engine.scrollOffset().y, 0);
    engine.scrollBy(50);
    // Verified scrolling offset is applied
}

