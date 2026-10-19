#include "test_framework.hpp"
#include "browser/platform/window.hpp"
#include "browser/events/event_dispatcher.hpp"

using namespace browser::platform;
using namespace browser::events;

TEST_CASE("BrowserWindow: Headless window initialization and teardown") {
    WindowConfig config{
        .title = "Test Headless Window",
        .width = 800,
        .height = 600,
        .resizable = true,
        .headless = true
    };

    BrowserWindow window(config);
    REQUIRE(window.initialize());

    CHECK(window.isOpen());
    CHECK(window.isHeadless());
    CHECK_EQ(window.size().width, 800);
    CHECK_EQ(window.size().height, 600);

    // Test clear and present in headless mode
    window.clear(browser::core::Color::white());
    window.present();

    // Test drawing primitives
    window.drawRect(browser::core::Rect{10, 10, 100, 100}, browser::core::Color::red(), true);
    window.drawLine(browser::core::Point{0, 0}, browser::core::Point{100, 100}, browser::core::Color::black());
    window.present();

    // Test event polling
    EventDispatcher dispatcher;
    window.pollEvents(dispatcher);

    window.close();
    CHECK_FALSE(window.isOpen());
}

