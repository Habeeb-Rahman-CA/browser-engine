#include "browser/shell/frame_scheduler.hpp"
#include "browser/core/diagnostics.hpp"

#include <thread>
#include <algorithm>

namespace browser::shell {

FrameScheduler::FrameScheduler(double targetFps) {
    setTargetFps(targetFps);
    reset();
}

void FrameScheduler::setTargetFps(double targetFps) noexcept {
    m_targetFps = targetFps;
    if (m_targetFps > 0.0) {
        auto ns = static_cast<int64_t>(1'000'000'000.0 / m_targetFps);
        m_targetFrameDuration = std::chrono::nanoseconds(ns);
    } else {
        m_targetFrameDuration = std::chrono::nanoseconds(0);
    }
}

void FrameScheduler::reset() {
    auto now = std::chrono::steady_clock::now();
    m_frameStart = now;
    m_lastFrameTime = now;
    m_fpsLastCalculationTime = now;
    m_fpsFramesCounted = 0;
    m_frameCount = 0;
    m_deltaTime = (m_targetFps > 0.0) ? (1.0 / m_targetFps) : 0.0166667;
    m_currentFps = m_targetFps;
}

void FrameScheduler::beginFrame() {
    auto now = std::chrono::steady_clock::now();
    if (m_frameCount > 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - m_lastFrameTime);
        m_deltaTime = std::clamp(elapsed.count(), 0.0001, 0.25); // Clamp dt to prevent spiral of death
    }
    m_lastFrameTime = now;
    m_frameStart = now;
}

void FrameScheduler::endFrame() {
    auto workDoneTime = std::chrono::steady_clock::now();
    auto workDuration = workDoneTime - m_frameStart;

    // Throttle to target FPS if we have time remaining
    if (m_targetFrameDuration.count() > 0 && workDuration < m_targetFrameDuration) {
        auto remaining = m_targetFrameDuration - workDuration;
        std::this_thread::sleep_for(remaining);
    }

    auto frameEndTime = std::chrono::steady_clock::now();
    auto totalFrameDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(frameEndTime - m_frameStart);

    m_frameCount++;
    m_fpsFramesCounted++;

    // Calculate smoothed FPS every 0.5s
    auto fpsElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(frameEndTime - m_fpsLastCalculationTime).count();
    if (fpsElapsed >= 0.5) {
        m_currentFps = static_cast<double>(m_fpsFramesCounted) / fpsElapsed;
        m_fpsFramesCounted = 0;
        m_fpsLastCalculationTime = frameEndTime;
    }

    // Diagnostics integration
    core::BrowserDiagnostics::instance().recordDuration(
        core::Subsystem::FrameTotal,
        totalFrameDuration
    );
    core::BrowserDiagnostics::instance().incrementCounter("FramesRendered");
}

} // namespace browser::shell

