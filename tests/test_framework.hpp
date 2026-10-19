#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>

namespace test {

struct TestContext {
    int totalAssertions{0};
    int passedAssertions{0};
    int failedAssertions{0};
    std::string currentTestName;
    bool currentTestFailed{false};

    static TestContext& instance() {
        static TestContext s_instance;
        return s_instance;
    }
};

using TestFunction = std::function<void()>;

struct TestCase {
    std::string name;
    TestFunction func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry s_registry;
        return s_registry;
    }

    void registerTest(std::string name, TestFunction func) {
        m_tests.push_back({std::move(name), std::move(func)});
    }

    int runAll() {
        auto& ctx = TestContext::instance();
        int testsRun = 0;
        int testsPassed = 0;
        int testsFailed = 0;

        std::cout << "\033[1;36m====================================================\033[0m\n";
        std::cout << "\033[1;36m                RUNNING BROWSER UNIT TESTS           \033[0m\n";
        std::cout << "\033[1;36m====================================================\033[0m\n";

        for (const auto& testCase : m_tests) {
            ctx.currentTestName = testCase.name;
            ctx.currentTestFailed = false;
            testsRun++;

            try {
                testCase.func();
            } catch (const std::exception& e) {
                ctx.currentTestFailed = true;
                ctx.failedAssertions++;
                std::cerr << "  \033[1;31m[EXCEPTION]\033[0m " << testCase.name
                          << " threw std::exception: " << e.what() << "\n";
            } catch (...) {
                ctx.currentTestFailed = true;
                ctx.failedAssertions++;
                std::cerr << "  \033[1;31m[EXCEPTION]\033[0m " << testCase.name
                          << " threw unknown exception!\n";
            }

            if (ctx.currentTestFailed) {
                testsFailed++;
                std::cout << "  \033[1;31mFAILED\033[0m: " << testCase.name << "\n";
            } else {
                testsPassed++;
                std::cout << "  \033[1;32mPASSED\033[0m: " << testCase.name << "\n";
            }
        }

        std::cout << "\033[1;36m====================================================\033[0m\n";
        std::cout << "Test Summary: "
                  << testsPassed << "/" << testsRun << " test cases passed ("
                  << ctx.passedAssertions << " assertions passed, "
                  << ctx.failedAssertions << " failed)\n";
        std::cout << "\033[1;36m====================================================\033[0m\n";

        return testsFailed == 0 ? 0 : 1;
    }

private:
    std::vector<TestCase> m_tests;
};

struct TestRegistrar {
    TestRegistrar(std::string name, TestFunction func) {
        TestRegistry::instance().registerTest(std::move(name), std::move(func));
    }
};

inline void recordAssertion(bool success, const char* expr, const char* file, int line) {
    auto& ctx = TestContext::instance();
    ctx.totalAssertions++;
    if (success) {
        ctx.passedAssertions++;
    } else {
        ctx.failedAssertions++;
        ctx.currentTestFailed = true;
        std::cerr << "    \033[1;31mCHECK FAILED\033[0m at " << file << ":" << line
                  << " -> Expression: (" << expr << ")\n";
    }
}

template <typename A, typename B>
inline void recordEqAssertion(const A& a, const B& b, bool success, const char* exprA, const char* exprB, const char* file, int line) {
    auto& ctx = TestContext::instance();
    ctx.totalAssertions++;
    if (success) {
        ctx.passedAssertions++;
    } else {
        ctx.failedAssertions++;
        ctx.currentTestFailed = true;
        std::cerr << "    \033[1;31mCHECK_EQ FAILED\033[0m at " << file << ":" << line
                  << " -> " << exprA << " == " << exprB
                  << " (Values: " << a << " vs " << b << ")\n";
    }
}

} // namespace test

#define TEST_CONCAT_INNER(a, b) a##b
#define TEST_CONCAT(a, b) TEST_CONCAT_INNER(a, b)

#define TEST_CASE_INTERNAL(name, id) \
    static void TEST_CONCAT(test_func_, id)(); \
    static ::test::TestRegistrar TEST_CONCAT(test_registrar_, id)(name, TEST_CONCAT(test_func_, id)); \
    static void TEST_CONCAT(test_func_, id)()

#define TEST_CASE(name) TEST_CASE_INTERNAL(name, __COUNTER__)

#define CHECK(expr) ::test::recordAssertion(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_FALSE(expr) ::test::recordAssertion(!static_cast<bool>(expr), "!" #expr, __FILE__, __LINE__)
#define CHECK_EQ(a, b) ::test::recordEqAssertion((a), (b), ((a) == (b)), #a, #b, __FILE__, __LINE__)
#define CHECK_NE(a, b) ::test::recordAssertion(((a) != (b)), #a " != " #b, __FILE__, __LINE__)
#define REQUIRE(expr) do { \
    bool _req_ok = static_cast<bool>(expr); \
    ::test::recordAssertion(_req_ok, #expr, __FILE__, __LINE__); \
    if (!_req_ok) { return; } \
} while (false)
