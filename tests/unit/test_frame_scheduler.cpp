#include "test_framework.hpp"
#include "browser/shell/frame_scheduler.hpp"

#include <thread>

using namespace browser::shell;

TEST_CASE("FrameScheduler: Target FPS and frame duration configuration") {
    FrameScheduler scheduler(60.0);
    CHECK_EQ(scheduler.targetFps(), 60.0);

    scheduler.setTargetFps(120.0);
    CHECK_EQ(scheduler.targetFps(), 120.0);

    scheduler.setTargetFps(30.0);
    CHECK_EQ(scheduler.targetFps(), 30.0);
}

TEST_CASE("FrameScheduler: Advances frame counts and delta times") {
    // Use high target FPS for test speed
    FrameScheduler scheduler(240.0);
    CHECK_EQ(scheduler.frameCount(), 0u);

    scheduler.beginFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    scheduler.endFrame();

    CHECK_EQ(scheduler.frameCount(), 1u);
    CHECK(scheduler.deltaTime() > 0.0);

    scheduler.beginFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    scheduler.endFrame();

    CHECK_EQ(scheduler.frameCount(), 2u);
}

