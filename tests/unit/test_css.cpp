#include "test_framework.hpp"
#include "browser/css/css_tokenizer.hpp"
#include "browser/css/css_parser.hpp"
#include "browser/style/selector_matcher.hpp"
#include "browser/style/style_resolver.hpp"
#include "browser/html/parser.hpp"

using namespace browser::css;
using namespace browser::style;
using namespace browser::html;

TEST_CASE("CSS: Tokenizer parses identifiers, dimensions, and colors") {
    CssTokenizer tokenizer("h1 { color: #ff0000; font-size: 24px; width: 100%; /* comment */ }");
    auto tokens = tokenizer.tokenizeAll();

    bool foundIdent = false;
    bool foundHash = false;
    bool foundDimension = false;
    bool foundPercent = false;

    for (const auto& tok : tokens) {
        if (tok.type == CssTokenType::Ident && tok.text == "h1") foundIdent = true;
        if (tok.type == CssTokenType::Hash && tok.text == "ff0000") foundHash = true;
        if (tok.type == CssTokenType::Dimension && tok.numberValue == 24.0 && tok.unit == "px") foundDimension = true;
        if (tok.type == CssTokenType::Percentage && tok.numberValue == 100.0) foundPercent = true;
    }

    CHECK(foundIdent);
    CHECK(foundHash);
    CHECK(foundDimension);
    CHECK(foundPercent);
}

TEST_CASE("CSS: Parser extracts rules, selectors, and declarations") {
    std::string css = 
        ".card { padding: 16px; background-color: #ffffff; }\n"
        "div > p.lead { font-size: 20px; font-weight: bold; }";

    auto sheet = CssParser::parse(css);
    REQUIRE(sheet.rules.size() == 2u);

    // Rule 1: .card
    const auto& rule1 = sheet.rules[0];
    REQUIRE(rule1.selectors.size() == 1u);
    CHECK_EQ(rule1.selectors[0].specificity().b, 1u); // class = (0, 1, 0)
    CHECK_EQ(rule1.declarations.size(), 2u);
    CHECK_EQ(rule1.declarations[0].property, "padding");
    CHECK_EQ(rule1.declarations[0].value, "16px");

    // Rule 2: div > p.lead
    const auto& rule2 = sheet.rules[1];
    REQUIRE(rule2.selectors.size() == 1u);
    auto spec2 = rule2.selectors[0].specificity();
    CHECK_EQ(spec2.b, 1u); // 1 class (.lead)
    CHECK_EQ(spec2.c, 2u); // 2 elements (div, p)
}

TEST_CASE("CSS: Specificity comparison orders correctly") {
    Specificity idSpec{1, 0, 0};
    Specificity classSpec{0, 2, 1};
    Specificity elementSpec{0, 0, 5};

    CHECK(idSpec > classSpec);
    CHECK(classSpec > elementSpec);
    CHECK(idSpec > elementSpec);
}

TEST_CASE("Style: SelectorMatcher tests hierarchy and combinators") {
    auto doc = HtmlParser::parseDocument(
        "<div id=\"main\" class=\"container\">\n"
        "  <ul class=\"list\">\n"
        "    <li class=\"item active\"><a href=\"#\">Link</a></li>\n"
        "  </ul>\n"
        "</div>"
    );

    auto linkEl = doc->getElementsByTagName("a")[0];
    auto liEl = doc->getElementsByTagName("li")[0];
    auto ulEl = doc->getElementsByTagName("ul")[0];
    auto divEl = doc->getElementById("main");

    // 1. Simple type match
    auto selA = CssParser::parse("a {}").rules[0].selectors[0];
    CHECK(SelectorMatcher::matches(*linkEl, selA));

    // 2. Class match
    auto selActive = CssParser::parse(".active {}").rules[0].selectors[0];
    CHECK(SelectorMatcher::matches(*liEl, selActive));

    // 3. Child combinator (ul > li)
    auto selChild = CssParser::parse("ul > li {}").rules[0].selectors[0];
    CHECK(SelectorMatcher::matches(*liEl, selChild));

    // 4. Descendant combinator (#main a)
    auto selDesc = CssParser::parse("#main a {}").rules[0].selectors[0];
    CHECK(SelectorMatcher::matches(*linkEl, selDesc));

    // 5. Non-matching selector
    auto selNoMatch = CssParser::parse("div > a {}").rules[0].selectors[0]; // a is not immediate child of div
    CHECK_FALSE(SelectorMatcher::matches(*linkEl, selNoMatch));
}

TEST_CASE("Style: StyleResolver applies cascade, specificity, and inheritance") {
    std::string html = 
        "<div id=\"container\" class=\"box\" style=\"color: #00ff00;\">\n"
        "  <p class=\"text\">Paragraph content</p>\n"
        "</div>";

    std::string css = 
        "div { color: #ff0000; font-size: 18px; }\n"
        ".box { color: #0000ff; font-weight: bold; }\n"
        "#container { font-size: 20px; }\n"
        "p { margin: 10px 20px; }";

    auto doc = HtmlParser::parseDocument(html);
    StyleResolver resolver;
    resolver.addAuthorStyleSheet(CssParser::parse(css));

    auto divEl = doc->getElementById("container");
    REQUIRE(divEl != nullptr);

    auto divStyle = resolver.resolveStyle(*divEl, nullptr);
    // Inline style (color: #00ff00) overrides #container, .box, and div
    CHECK_EQ(divStyle->color.g, 255);
    CHECK_EQ(divStyle->color.r, 0);

    // ID selector (#container font-size: 20px) overrides div (18px)
    CHECK_EQ(divStyle->fontSize, 20.0f);
    CHECK_EQ(divStyle->fontWeight, 700); // from .box bold

    // Resolve paragraph child: inherits color and font-size
    auto pEl = doc->getElementsByTagName("p")[0];
    auto pStyle = resolver.resolveStyle(*pEl, divStyle.get());

    CHECK_EQ(pStyle->color.g, 255);       // Inherited from div
    CHECK_EQ(pStyle->fontSize, 20.0f);    // Inherited from div
    CHECK_EQ(pStyle->display, Display::Block); // from UA stylesheet
    CHECK_EQ(pStyle->margin.top.value, 10.0f);
    CHECK_EQ(pStyle->margin.right.value, 20.0f);
}
