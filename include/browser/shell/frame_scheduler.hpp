#pragma once

#include <chrono>
#include <cstdint>

namespace browser::shell {

class FrameScheduler {
public:
    explicit FrameScheduler(double targetFps = 60.0);

    void setTargetFps(double targetFps) noexcept;
    [[nodiscard]] double targetFps() const noexcept { return m_targetFps; }

    void beginFrame();
    void endFrame();

    [[nodiscard]] double deltaTime() const noexcept { return m_deltaTime; }
    [[nodiscard]] double currentFps() const noexcept { return m_currentFps; }
    [[nodiscard]] uint64_t frameCount() const noexcept { return m_frameCount; }

    void reset();

private:
    double m_targetFps{60.0};
    std::chrono::nanoseconds m_targetFrameDuration{16'666'666}; // ~60fps in ns

    std::chrono::steady_clock::time_point m_frameStart;
    std::chrono::steady_clock::time_point m_lastFrameTime;

    double m_deltaTime{0.0166667}; // Default 60fps dt
    double m_currentFps{0.0};
    uint64_t m_frameCount{0};

    // FPS calculation smoothing
    uint32_t m_fpsFramesCounted{0};
    std::chrono::steady_clock::time_point m_fpsLastCalculationTime;
};

} // namespace browser::shell

