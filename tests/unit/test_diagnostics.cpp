#include "test_framework.hpp"
#include "browser/core/diagnostics.hpp"

#include <thread>
#include <sstream>

using namespace browser::core;

TEST_CASE("Diagnostics: Counters can be incremented and retrieved") {
    auto& diag = BrowserDiagnostics::instance();
    diag.reset();

    CHECK_EQ(diag.counter("test_counter"), 0u);

    diag.incrementCounter("test_counter", 5);
    CHECK_EQ(diag.counter("test_counter"), 5u);

    diag.incrementCounter("test_counter", 3);
    CHECK_EQ(diag.counter("test_counter"), 8u);

    diag.setCounter("test_counter", 42);
    CHECK_EQ(diag.counter("test_counter"), 42u);
}

TEST_CASE("Diagnostics: Subsystem metrics record duration and call counts") {
    auto& diag = BrowserDiagnostics::instance();
    diag.reset();

    diag.recordDuration(Subsystem::HtmlTokenizer, std::chrono::milliseconds(10));
    diag.recordDuration(Subsystem::HtmlTokenizer, std::chrono::milliseconds(20));

    auto metric = diag.metric(Subsystem::HtmlTokenizer);
    CHECK_EQ(metric.callCount, 2u);
    CHECK(metric.totalMilliseconds() >= 29.9);
    CHECK(metric.averageMilliseconds() >= 14.9);
}

TEST_CASE("Diagnostics: ScopedTimer measures elapsed time correctly") {
    auto& diag = BrowserDiagnostics::instance();
    diag.reset();

    {
        ScopedTimer timer(Subsystem::Layout);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto metric = diag.metric(Subsystem::Layout);
    CHECK_EQ(metric.callCount, 1u);
    CHECK(metric.totalMilliseconds() >= 8.0);
}

TEST_CASE("Diagnostics: Summary table output contains subsystem metrics") {
    auto& diag = BrowserDiagnostics::instance();
    diag.reset();

    diag.recordDuration(Subsystem::Painting, std::chrono::milliseconds(5));
    diag.incrementCounter("DrawCalls", 15);

    std::ostringstream ss;
    diag.dumpSummary(ss);
    std::string output = ss.str();

    CHECK(output.find("Painting") != std::string::npos);
    CHECK(output.find("DrawCalls") != std::string::npos);
    CHECK(output.find("15") != std::string::npos);
}

int main() {
    return test::TestRegistry::instance().runAll();
}

