#include "test_framework.hpp"
#include "browser/dom/document.hpp"
#include "browser/html/parser.hpp"

#include <sstream>

using namespace browser::dom;
using namespace browser::html;

TEST_CASE("DOM: Node hierarchy, appendChild, and removeChild") {
    auto doc = std::make_shared<Document>();
    auto parent = doc->createElement("div");
    auto child1 = doc->createElement("span");
    auto child2 = doc->createElement("p");

    parent->appendChild(child1);
    parent->appendChild(child2);

    CHECK_EQ(parent->childNodes().size(), 2u);
    CHECK(child1->parentNode() == parent);
    CHECK(child2->parentNode() == parent);
    CHECK(parent->firstChild() == child1);
    CHECK(parent->lastChild() == child2);
    CHECK(child1->nextSibling() == child2);
    CHECK(child2->previousSibling() == child1);

    // Remove child
    parent->removeChild(child1);
    CHECK_EQ(parent->childNodes().size(), 1u);
    CHECK(child1->parentNode() == nullptr);
    CHECK(parent->firstChild() == child2);
}

TEST_CASE("DOM: Node insertBefore") {
    auto doc = std::make_shared<Document>();
    auto parent = doc->createElement("ul");
    auto li1 = doc->createElement("li");
    auto li2 = doc->createElement("li");
    auto li0 = doc->createElement("li");

    parent->appendChild(li1);
    parent->appendChild(li2);

    // Insert li0 before li1
    parent->insertBefore(li0, li1);

    CHECK_EQ(parent->childNodes().size(), 3u);
    CHECK(parent->firstChild() == li0);
    CHECK(li0->nextSibling() == li1);
}

TEST_CASE("DOM: Element attributes and classList") {
    auto doc = std::make_shared<Document>();
    auto el = doc->createElement("button");

    el->setAttribute("id", "submit-btn");
    el->setAttribute("class", "btn btn-primary active");
    el->setAttribute("type", "submit");

    CHECK_EQ(el->id(), "submit-btn");
    CHECK_EQ(el->className(), "btn btn-primary active");
    CHECK(el->hasAttribute("type"));
    CHECK_EQ(el->getAttribute("type"), "submit");

    auto classes = el->classList();
    REQUIRE(classes.size() == 3);
    CHECK_EQ(classes[0], "btn");
    CHECK_EQ(classes[1], "btn-primary");
    CHECK_EQ(classes[2], "active");
}

TEST_CASE("DOM: getElementById and getElementsByTagName") {
    auto doc = std::make_shared<Document>();
    auto html = doc->createElement("html");
    auto body = doc->createElement("body");
    auto nav = doc->createElement("nav");
    nav->setAttribute("id", "main-nav");

    auto a1 = doc->createElement("a");
    a1->setTextContent("Home");
    auto a2 = doc->createElement("a");
    a2->setTextContent("About");

    nav->appendChild(a1);
    nav->appendChild(a2);
    body->appendChild(nav);
    html->appendChild(body);
    doc->appendChild(html);
    doc->setDocumentElement(html);

    // Test getElementById
    auto found = doc->getElementById("main-nav");
    REQUIRE(found != nullptr);
    CHECK_EQ(found->tagName(), "nav");

    // Test getElementsByTagName
    auto links = doc->getElementsByTagName("a");
    CHECK_EQ(links.size(), 2u);
    CHECK_EQ(links[0]->textContent(), "Home");
    CHECK_EQ(links[1]->textContent(), "About");
}

TEST_CASE("HtmlParser: Parse full HTML document") {
    std::string html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>My Browser</title></head>\n"
        "<body>\n"
        "  <div id=\"content\">\n"
        "    <h1>Welcome</h1>\n"
        "    <p>This is a paragraph.</p>\n"
        "  </div>\n"
        "</body>\n"
        "</html>";

    auto doc = HtmlParser::parseDocument(html);
    REQUIRE(doc != nullptr);
    REQUIRE(doc->documentElement() != nullptr);
    CHECK_EQ(doc->documentElement()->tagName(), "html");

    CHECK_EQ(doc->title(), "My Browser");
    REQUIRE(doc->body() != nullptr);

    auto contentDiv = doc->getElementById("content");
    REQUIRE(contentDiv != nullptr);
    CHECK_EQ(contentDiv->tagName(), "div");

    auto headings = doc->getElementsByTagName("h1");
    REQUIRE(headings.size() == 1u);
    CHECK_EQ(headings[0]->textContent(), "Welcome");
}

TEST_CASE("HtmlParser: Implicit tags and auto-closing paragraph recovery") {
    // Malformed HTML with missing html/head/body and unclosed <p> tags
    std::string html = 
        "<p>First paragraph\n"
        "<p>Second paragraph\n"
        "<div>A div box</div>\n"
        "<img src=\"photo.jpg\">\n";

    auto doc = HtmlParser::parseDocument(html);
    REQUIRE(doc != nullptr);
    REQUIRE(doc->documentElement() != nullptr);
    REQUIRE(doc->body() != nullptr);

    auto paras = doc->getElementsByTagName("p");
    CHECK_EQ(paras.size(), 2u);

    auto divs = doc->getElementsByTagName("div");
    CHECK_EQ(divs.size(), 1u);

    auto imgs = doc->getElementsByTagName("img");
    CHECK_EQ(imgs.size(), 1u);
    CHECK_EQ(imgs[0]->getAttribute("src"), "photo.jpg");
}

TEST_CASE("DOM: Tree ASCII dump inspection") {
    auto doc = HtmlParser::parseDocument("<html><body><div id=\"card\"><p>Hello World</p></div></body></html>");
    REQUIRE(doc != nullptr);

    std::ostringstream ss;
    doc->dumpTree(ss);
    std::string treeStr = ss.str();

    CHECK(treeStr.find("<html") != std::string::npos);
    CHECK(treeStr.find("<body") != std::string::npos);
    CHECK(treeStr.find("<div") != std::string::npos);
    CHECK(treeStr.find("<p") != std::string::npos);
    CHECK(treeStr.find("Hello World") != std::string::npos);
}

