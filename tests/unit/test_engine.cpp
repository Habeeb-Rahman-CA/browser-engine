#include "test_framework.hpp"
#include "browser/shell/engine.hpp"

using namespace browser::shell;

TEST_CASE("BrowserEngine: Headless initialization and step loop") {
    EngineConfig config{
        .window = {
            .title = "Test Engine",
            .width = 640,
            .height = 480,
            .resizable = false,
            .headless = true
        },
        .initialUrl = "about:blank",
        .dumpDiagnosticsOnExit = false,
        .maxFrames = 5,
        .verbose = false
    };

    BrowserEngine engine(config);
    REQUIRE(engine.initialize());

    // Execute 3 steps manually
    engine.step();
    engine.step();
    engine.step();

    CHECK_EQ(engine.scheduler().frameCount(), 3u);

    // Run remaining frames to verify maxFrames termination
    int exitCode = engine.run();
    CHECK_EQ(exitCode, 0);
    CHECK(engine.scheduler().frameCount() >= 5u);
}

