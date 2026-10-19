#include "test_framework.hpp"
#include "browser/javascript/lexer.hpp"
#include "browser/javascript/parser.hpp"
#include "browser/javascript/interpreter.hpp"

using namespace browser::javascript;

TEST_CASE("JavaScript: Lexer tokenizes identifiers, literals, and comments") {
    Lexer lexer("let x = 42; // comment\n/* block */ const msg = \"hello world\";");
    auto tokens = lexer.tokenize();

    REQUIRE(tokens.size() == 11);
    CHECK(tokens[0].type == TokenType::Let);
    CHECK_EQ(tokens[1].text, "x");
    CHECK(tokens[2].type == TokenType::Equal);
    CHECK_EQ(tokens[3].numberValue, 42.0);
    CHECK(tokens[4].type == TokenType::Semicolon);
    CHECK(tokens[5].type == TokenType::Const);
    CHECK_EQ(tokens[6].text, "msg");
    CHECK(tokens[7].type == TokenType::Equal);
    CHECK_EQ(tokens[8].text, "hello world");
    CHECK(tokens[9].type == TokenType::Semicolon);
    CHECK(tokens[10].type == TokenType::EndOfFile);
}

TEST_CASE("JavaScript: Arithmetic and operator precedence") {
    Interpreter interp;
    Value result = interp.evaluate("let a = 2 + 3 * 4; a;");
    CHECK(result.isNumber());
    CHECK_EQ(result.asNumber(), 14.0);

    Value p = interp.evaluate("(2 + 3) * 4;");
    CHECK_EQ(p.asNumber(), 20.0);

    Value s = interp.evaluate("\"foo\" + \" \" + \"bar\";");
    CHECK_EQ(s.asString(), "foo bar");
}

TEST_CASE("JavaScript: Control flow (for and while loops)") {
    Interpreter interp;
    interp.evaluate(R"(
        let sum = 0;
        for (let i = 1; i <= 10; i = i + 1) {
            sum = sum + i;
        }
    )");
    Value sum = interp.getGlobal("sum");
    CHECK_EQ(sum.asNumber(), 55.0);

    interp.evaluate(R"(
        let count = 0;
        let n = 5;
        while (n > 0) {
            count = count + 2;
            n = n - 1;
        }
    )");
    Value count = interp.getGlobal("count");
    CHECK_EQ(count.asNumber(), 10.0);
}

TEST_CASE("JavaScript: Function declarations, returns, and closures") {
    Interpreter interp;
    interp.evaluate(R"(
        function add(a, b) {
            return a + b;
        }
        let res = add(15, 27);
    )");
    CHECK_EQ(interp.getGlobal("res").asNumber(), 42.0);

    // Closures
    interp.evaluate(R"(
        function makeCounter(start) {
            let current = start;
            function count() {
                current = current + 1;
                return current;
            }
            return count;
        }
        let counterA = makeCounter(10);
        let val1 = counterA();
        let val2 = counterA();
    )");
    CHECK_EQ(interp.getGlobal("val1").asNumber(), 11.0);
    CHECK_EQ(interp.getGlobal("val2").asNumber(), 12.0);
}

TEST_CASE("JavaScript: Infinite loop safety via execution step limits") {
    InterpreterLimits limits;
    limits.maxSteps = 500;
    Interpreter interp(limits);

    // Infinite loop should trigger step limit and stop safely
    Value result = interp.evaluate("while (true) {}");
    CHECK(result.isUndefined());
    CHECK(interp.stepCount() > limits.maxSteps);
}
