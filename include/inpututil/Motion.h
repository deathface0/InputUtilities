#pragma once

#include <chrono>
#include <cstdint>

namespace inpututil {

/// Speed profile of a movement over its duration.
enum class Easing : std::uint8_t {
    Linear,         ///< Constant speed.
    SmoothStep,     ///< Gentle acceleration and deceleration.
    EaseInOutCubic, ///< Stronger acceleration and deceleration; closest to a hand movement.
};

/// Progress (0..1) reached at time t (0..1) with the given easing.
double easingValue(Easing easing, double t) noexcept;

/// How a pointer movement unfolds over time. Duration based: the movement
/// takes `duration`, however long each individual wait actually lasts.
///
///     input.mouse.moveTo({800, 400}, Motion::human(400ms));
///     input.mouse.moveTo({800, 400}, {.duration = 300ms, .easing = Easing::Linear});
struct Motion {
    /// Total time of the movement; zero moves instantly.
    std::chrono::milliseconds duration{0};

    Easing easing = Easing::SmoothStep;

    /// Follow a randomly bent Bézier curve instead of a straight line.
    bool curved = false;

    /// Random offset, in pixels, added to every intermediate point; 0 or less adds none.
    int jitterPx = 0;

    /// Time between two points (at least 1 ms).
    std::chrono::milliseconds stepInterval{5};

    /// Seed for the curve and the jitter; 0 picks a random one, any other value is reproducible.
    std::uint32_t seed = 0;

    static constexpr Motion instant() noexcept { return Motion{}; }

    static constexpr Motion linear(std::chrono::milliseconds duration) noexcept {
        return {.duration = duration, .easing = Easing::Linear};
    }

    static constexpr Motion smooth(std::chrono::milliseconds duration) noexcept {
        return {.duration = duration, .easing = Easing::SmoothStep};
    }

    /// Curved path, natural acceleration and a pixel of jitter.
    static constexpr Motion human(std::chrono::milliseconds duration) noexcept {
        return {.duration = duration, .easing = Easing::EaseInOutCubic, .curved = true, .jitterPx = 1};
    }
};

} // namespace inpututil
