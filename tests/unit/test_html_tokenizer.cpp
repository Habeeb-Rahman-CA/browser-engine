#include "test_framework.hpp"
#include "browser/html/tokenizer.hpp"

using namespace browser::html;

TEST_CASE("HtmlTokenizer: DOCTYPE declaration") {
    HtmlTokenizer tokenizer("<!DOCTYPE html>");
    auto tokens = tokenizer.tokenizeAll();

    REQUIRE(tokens.size() >= 2); // DOCTYPE + EOF
    CHECK(tokens[0].type == TokenType::DOCTYPE);
    CHECK_EQ(tokens[0].data, "html");
    CHECK(tokens[1].type == TokenType::EndOfFile);
}

TEST_CASE("HtmlTokenizer: Simple start and end tags") {
    HtmlTokenizer tokenizer("<h1>Hello</h1>");
    auto tokens = tokenizer.tokenizeAll();

    REQUIRE(tokens.size() == 4); // StartTag, Character, EndTag, EOF
    CHECK(tokens[0].type == TokenType::StartTag);
    CHECK_EQ(tokens[0].tagName, "h1");

    CHECK(tokens[1].type == TokenType::Character);
    CHECK_EQ(tokens[1].data, "Hello");

    CHECK(tokens[2].type == TokenType::EndTag);
    CHECK_EQ(tokens[2].tagName, "h1");

    CHECK(tokens[3].type == TokenType::EndOfFile);
}

TEST_CASE("HtmlTokenizer: Case insensitivity normalization") {
    HtmlTokenizer tokenizer("<DIV ID=\"main\" CLASS=\"hero\"></DIV>");
    auto tokens = tokenizer.tokenizeAll();

    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0].type == TokenType::StartTag);
    CHECK_EQ(tokens[0].tagName, "div");
    CHECK(tokens[0].hasAttribute("id"));
    CHECK(tokens[0].hasAttribute("class"));
    CHECK_EQ(tokens[0].getAttribute("id").value(), "main");
    CHECK_EQ(tokens[0].getAttribute("class").value(), "hero");

    CHECK(tokens[1].type == TokenType::EndTag);
    CHECK_EQ(tokens[1].tagName, "div");
}

TEST_CASE("HtmlTokenizer: Single quoted and unquoted attributes") {
    HtmlTokenizer tokenizer("<input type='checkbox' name=agree value=yes checked>");
    auto tokens = tokenizer.tokenizeAll();

    REQUIRE(tokens.size() == 2); // StartTag + EOF
    const auto& tok = tokens[0];
    CHECK(tok.type == TokenType::StartTag);
    CHECK_EQ(tok.tagName, "input");
    CHECK_EQ(tok.getAttribute("type").value(), "checkbox");
    CHECK_EQ(tok.getAttribute("name").value(), "agree");
    CHECK_EQ(tok.getAttribute("value").value(), "yes");
    CHECK(tok.hasAttribute("checked"));
}

TEST_CASE("HtmlTokenizer: Void and self-closing elements") {
    HtmlTokenizer tokenizer("<br><hr/><img src=\"image.png\">");
    auto tokens = tokenizer.tokenizeAll();

    REQUIRE(tokens.size() == 4); // br, hr, img, EOF
    CHECK(tokens[0].type == TokenType::StartTag);
    CHECK_EQ(tokens[0].tagName, "br");
    CHECK(tokens[0].selfClosing);

    CHECK(tokens[1].type == TokenType::StartTag);
    CHECK_EQ(tokens[1].tagName, "hr");
    CHECK(tokens[1].selfClosing);

    CHECK(tokens[2].type == TokenType::StartTag);
    CHECK_EQ(tokens[2].tagName, "img");
    CHECK(tokens[2].selfClosing);
    CHECK_EQ(tokens[2].getAttribute("src").value(), "image.png");
}

TEST_CASE("HtmlTokenizer: Comments parsing") {
    HtmlTokenizer tokenizer("<!-- Standard comment -->\n<!-- Multi\nline\ncomment -->");
    auto tokens = tokenizer.tokenizeAll();

    // 2 comments + whitespace char tokens + EOF
    bool foundFirstComment = false;
    bool foundSecondComment = false;
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::Comment) {
            if (tok.data.find("Standard comment") != std::string::npos) {
                foundFirstComment = true;
            }
            if (tok.data.find("Multi") != std::string::npos) {
                foundSecondComment = true;
            }
        }
    }
    CHECK(foundFirstComment);
    CHECK(foundSecondComment);
}

TEST_CASE("HtmlTokenizer: Character entity and numeric references") {
    HtmlTokenizer tokenizer("<p>&lt;tag&gt; &amp; &quot;quote&quot; &#60; &#x3C;</p>");
    auto tokens = tokenizer.tokenizeAll();

    REQUIRE(tokens.size() >= 3);
    CHECK(tokens[0].type == TokenType::StartTag);

    // Verify text token contents contain decoded entities
    std::string textContent;
    for (size_t i = 1; i < tokens.size() - 2; ++i) {
        if (tokens[i].type == TokenType::Character) {
            textContent += tokens[i].data;
        }
    }

    CHECK(textContent.find("<tag>") != std::string::npos);
    CHECK(textContent.find("&") != std::string::npos);
    CHECK(textContent.find("\"quote\"") != std::string::npos);
    CHECK(textContent.find("<") != std::string::npos);
}

TEST_CASE("HtmlTokenizer: Malformed markup recovery") {
    // Stray '<', unclosed attribute, unclosed tag
    HtmlTokenizer tokenizer("2 < 3 and <span class=highlight>text");
    auto tokens = tokenizer.tokenizeAll();

    bool foundSpan = false;
    bool foundText = false;
    for (const auto& tok : tokens) {
        if (tok.type == TokenType::StartTag && tok.tagName == "span") {
            foundSpan = true;
            CHECK_EQ(tok.getAttribute("class").value(), "highlight");
        }
        if (tok.type == TokenType::Character && tok.data.find("text") != std::string::npos) {
            foundText = true;
        }
    }
    CHECK(foundSpan);
    CHECK(foundText);
}

