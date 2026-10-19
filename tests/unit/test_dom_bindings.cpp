#include "test_framework.hpp"
#include "browser/javascript/dom_bindings.hpp"
#include "browser/html/parser.hpp"

using namespace browser;

TEST_CASE("DOM Bindings: getElementById and getElementsByTagName in JS") {
    auto doc = html::HtmlParser::parseDocument(R"(
        <!DOCTYPE html>
        <html>
        <head><title>Binding Test</title></head>
        <body>
            <h1 id="heading">Hello World</h1>
            <p class="intro">First paragraph</p>
            <p class="intro">Second paragraph</p>
        </body>
        </html>
    )");

    javascript::Interpreter interp;
    javascript::DomBindings bindings(interp, doc);

    interp.evaluate(R"(
        let h1 = document.getElementById("heading");
        let titleText = h1.getTextContent();
        let paragraphs = document.getElementsByTagName("p");
        let count = paragraphs.length;
    )");

    CHECK_EQ(interp.getGlobal("titleText").asString(), "Hello World");
    CHECK_EQ(interp.getGlobal("count").asNumber(), 2.0);
}

TEST_CASE("DOM Bindings: JS mutations (setTextContent, setAttribute, appendChild)") {
    auto doc = html::HtmlParser::parseDocument(R"(
        <!DOCTYPE html>
        <html>
        <body>
            <div id="container"><p>Old text</p></div>
        </body>
        </html>
    )");

    javascript::Interpreter interp;
    javascript::DomBindings bindings(interp, doc);

    interp.evaluate(R"(
        let container = document.getElementById("container");
        container.setTextContent("New replaced text");
        container.setAttribute("data-test", "success");
    )");

    CHECK(bindings.hasMutations());
    auto container = doc->getElementById("container");
    REQUIRE(container != nullptr);
    CHECK_EQ(container->textContent(), "New replaced text");
    CHECK_EQ(container->getAttribute("data-test"), "success");

    // Dynamic element creation and appending
    interp.evaluate(R"(
        let newElem = document.createElement("span");
        newElem.setTextContent(" Appended child");
        let cont = document.getElementById("container");
        cont.appendChild(newElem);
    )");

    CHECK_EQ(container->childNodes().size(), 2);
    CHECK_EQ(container->textContent(), "New replaced text Appended child");
}

TEST_CASE("DOM Bindings: addEventListener and event dispatching") {
    auto doc = html::HtmlParser::parseDocument(R"(
        <!DOCTYPE html>
        <html>
        <body>
            <button id="btn">Click me</button>
        </body>
        </html>
    )");

    javascript::Interpreter interp;
    javascript::DomBindings bindings(interp, doc);

    interp.evaluate(R"(
        let clicked = 0;
        let btn = document.getElementById("btn");
        btn.addEventListener("click", function() {
            clicked = clicked + 1;
        });
    )");

    CHECK_EQ(interp.getGlobal("clicked").asNumber(), 0.0);

    // Simulate click event on button
    auto btnElem = doc->getElementById("btn");
    REQUIRE(btnElem != nullptr);

    bool handled = bindings.dispatchEvent(btnElem, "click");
    CHECK(handled);
    CHECK_EQ(interp.getGlobal("clicked").asNumber(), 1.0);

    // Click second time
    bindings.dispatchEvent(btnElem, "click");
    CHECK_EQ(interp.getGlobal("clicked").asNumber(), 2.0);
}

