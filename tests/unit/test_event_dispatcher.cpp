#include "test_framework.hpp"
#include "browser/events/event_dispatcher.hpp"

using namespace browser::events;
using namespace browser::platform;

TEST_CASE("EventDispatcher: Dispatches typed events to registered listeners") {
    EventDispatcher dispatcher;
    int receivedQuitCount = 0;
    int receivedResizeCount = 0;

    auto sub1 = dispatcher.subscribe(EventType::Quit, [&](const Event& ev) {
        CHECK(ev.type == EventType::Quit);
        receivedQuitCount++;
    });

    auto sub2 = dispatcher.subscribe(EventType::WindowResize, [&](const Event& ev) {
        CHECK(ev.type == EventType::WindowResize);
        if (const auto* resize = ev.get<WindowResizeEvent>()) {
            CHECK_EQ(resize->newSize.width, 1920);
            CHECK_EQ(resize->newSize.height, 1080);
        }
        receivedResizeCount++;
    });

    CHECK_EQ(dispatcher.listenerCount(), 2u);

    // Dispatch Quit event
    Event quitEvent{EventType::Quit, QuitEvent{}};
    dispatcher.dispatch(quitEvent);

    CHECK_EQ(receivedQuitCount, 1);
    CHECK_EQ(receivedResizeCount, 0);

    // Dispatch Resize event
    Event resizeEvent{EventType::WindowResize, WindowResizeEvent{browser::core::Size{1920, 1080}}};
    dispatcher.dispatch(resizeEvent);

    CHECK_EQ(receivedQuitCount, 1);
    CHECK_EQ(receivedResizeCount, 1);

    // Unsubscribe
    dispatcher.unsubscribe(sub1);
    dispatcher.unsubscribe(sub2);
    CHECK_EQ(dispatcher.listenerCount(), 0u);

    dispatcher.dispatch(quitEvent);
    CHECK_EQ(receivedQuitCount, 1); // Not incremented
}

TEST_CASE("EventDispatcher: Global listeners receive all event types") {
    EventDispatcher dispatcher;
    int totalEvents = 0;

    dispatcher.subscribeAll([&](const Event&) {
        totalEvents++;
    });

    dispatcher.dispatch(Event{EventType::Quit, QuitEvent{}});
    dispatcher.dispatch(Event{EventType::WindowFocus, WindowFocusEvent{true}});
    dispatcher.dispatch(Event{EventType::MouseMove, MouseMoveEvent{}});

    CHECK_EQ(totalEvents, 3);
}

TEST_CASE("EventDispatcher: Multiple listeners on same event type are all invoked") {
    EventDispatcher dispatcher;
    bool listener1Called = false;
    bool listener2Called = false;

    dispatcher.subscribe(EventType::KeyDown, [&](const Event&) {
        listener1Called = true;
    });

    dispatcher.subscribe(EventType::KeyDown, [&](const Event&) {
        listener2Called = true;
    });

    dispatcher.dispatch(Event{EventType::KeyDown, KeyEvent{}});

    CHECK(listener1Called);
    CHECK(listener2Called);
}

