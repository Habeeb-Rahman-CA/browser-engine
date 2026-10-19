#include "test_framework.hpp"
#include "browser/layout/layout_box.hpp"
#include "browser/layout/layout_tree_builder.hpp"
#include "browser/layout/layout_engine.hpp"
#include "browser/html/parser.hpp"
#include "browser/style/style_resolver.hpp"

#include <sstream>

using namespace browser::layout;
using namespace browser::style;
using namespace browser::html;

TEST_CASE("Layout: BoxDimensions geometry calculations") {
    BoxDimensions dims;
    dims.content = {10.0f, 20.0f, 200.0f, 100.0f};
    dims.padding = {5.0f, 10.0f, 5.0f, 10.0f};
    dims.border = {1.0f, 1.0f, 1.0f, 1.0f};
    dims.margin = {15.0f, 20.0f, 15.0f, 20.0f};

    auto pBox = dims.paddingBox();
    CHECK_EQ(pBox.x, 0.0f);
    CHECK_EQ(pBox.y, 15.0f);
    CHECK_EQ(pBox.width, 220.0f);
    CHECK_EQ(pBox.height, 110.0f);

    auto bBox = dims.borderBox();
    CHECK_EQ(bBox.x, -1.0f);
    CHECK_EQ(bBox.y, 14.0f);
    CHECK_EQ(bBox.width, 222.0f);
    CHECK_EQ(bBox.height, 112.0f);

    auto mBox = dims.marginBox();
    CHECK_EQ(mBox.x, -21.0f);
    CHECK_EQ(mBox.y, -1.0f);
    CHECK_EQ(mBox.width, 262.0f);
    CHECK_EQ(mBox.height, 142.0f);
}

TEST_CASE("Layout: Tree construction skips display:none elements") {
    std::string html = 
        "<html><head><title>Ignored</title><style>div { display: block; }</style></head>\n"
        "<body><div id=\"card\"><p>Visible</p></div></body></html>";

    auto doc = HtmlParser::parseDocument(html);
    StyleResolver resolver;
    resolver.loadStylesFromDocument(*doc);
    std::unordered_map<const browser::dom::Node*, std::shared_ptr<ComputedStyle>> styleMap;
    resolver.resolveTree(doc, styleMap);

    auto rootBox = LayoutTreeBuilder::build(doc, styleMap);
    REQUIRE(rootBox != nullptr);

    std::ostringstream ss;
    rootBox->dumpLayoutTree(ss);
    std::string dump = ss.str();

    CHECK(dump.find("head") == std::string::npos);
    CHECK(dump.find("title") == std::string::npos);
    CHECK(dump.find("<div") != std::string::npos);
    CHECK(dump.find("<p") != std::string::npos);
}

TEST_CASE("Layout: Block flow normal positioning and sizing") {
    std::string html = 
        "<html><body>\n"
        "  <div id=\"box1\" style=\"width: 400px; height: 100px; margin-bottom: 20px;\"></div>\n"
        "  <div id=\"box2\" style=\"width: 50%; height: 50px;\"></div>\n"
        "</body></html>";

    auto doc = HtmlParser::parseDocument(html);
    StyleResolver resolver;
    resolver.loadStylesFromDocument(*doc);
    std::unordered_map<const browser::dom::Node*, std::shared_ptr<ComputedStyle>> styleMap;
    resolver.resolveTree(doc, styleMap);

    auto rootBox = LayoutTreeBuilder::build(doc, styleMap);
    REQUIRE(rootBox != nullptr);

    // Layout in a 800x600 viewport
    LayoutEngine::layout(*rootBox, browser::core::Size{800, 600});

    // Traverse to body -> box1, box2
    auto bodyBox = rootBox->children()[0]->children()[0].get();
    REQUIRE(bodyBox->children().size() == 2u);

    auto box1 = bodyBox->children()[0].get();
    auto box2 = bodyBox->children()[1].get();

    // Box 1: fixed 400px width, 100px height
    CHECK_EQ(box1->dimensions().content.width, 400.0f);
    CHECK_EQ(box1->dimensions().content.height, 100.0f);
    CHECK_EQ(box1->dimensions().content.x, 0.0f);
    CHECK_EQ(box1->dimensions().content.y, 0.0f);

    // Box 2: 50% width of 800px viewport = 400px, 50px height
    CHECK_EQ(box2->dimensions().content.width, 400.0f);
    CHECK_EQ(box2->dimensions().content.height, 50.0f);
    // Positioned below box1 + box1's margin-bottom (20px) = 120px
    CHECK_EQ(box2->dimensions().content.y, 120.0f);
}

TEST_CASE("Layout: Auto margins center block elements") {
    std::string html = 
        "<html><body>\n"
        "  <div style=\"width: 400px; margin: 0 auto;\"></div>\n"
        "</body></html>";

    auto doc = HtmlParser::parseDocument(html);
    StyleResolver resolver;
    std::unordered_map<const browser::dom::Node*, std::shared_ptr<ComputedStyle>> styleMap;
    resolver.resolveTree(doc, styleMap);

    auto rootBox = LayoutTreeBuilder::build(doc, styleMap);
    REQUIRE(rootBox != nullptr);

    // 1000px viewport
    LayoutEngine::layout(*rootBox, browser::core::Size{1000, 800});

    auto bodyBox = rootBox->children()[0]->children()[0].get();
    auto centeredBox = bodyBox->children()[0].get();

    // Width is 400px, available is 1000px, remaining 600px split equally (300px margins each)
    CHECK_EQ(centeredBox->dimensions().content.width, 400.0f);
    CHECK_EQ(centeredBox->dimensions().margin.left, 300.0f);
    CHECK_EQ(centeredBox->dimensions().margin.right, 300.0f);
    CHECK_EQ(centeredBox->dimensions().content.x, 300.0f);
}

