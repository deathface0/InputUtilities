#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <utility>

#include "inpututil/Hold.h"
#include "inpututil/Motion.h"
#include "inpututil/Point.h"
#include "inpututil/Status.h"

namespace inpututil {

namespace detail {
class Session;
}

enum class MouseButton : std::uint8_t { Left, Right, Middle, X1, X2 };

struct ClickOptions {
    /// Number of clicks (2 = double click, 3 = triple click...).
    int count = 1;

    /// How long each click keeps the button down.
    std::chrono::milliseconds hold{0};

    /// Pause between two clicks.
    std::chrono::milliseconds interval{0};
};

/// Mouse of an Input. Positions are pixels of the virtual desktop (all
/// monitors; coordinates left of or above the primary monitor are negative).
///
///     input.mouse.moveTo({800, 400}, Motion::human(350ms));
///     input.mouse.moveRaw(300, 0, Motion::linear(200ms)); // turn a game camera
///     auto drag = input.mouse.hold(MouseButton::Left);
class Mouse {
public:
    Mouse(const Mouse&) = delete;
    Mouse& operator=(const Mouse&) = delete;

    /// Current cursor position, or nullopt if it cannot be read.
    std::optional<Point> position() const;

    /// Moves to an exact pixel (clamped to the virtual desktop). A motion with
    /// a duration starts from the current position.
    Status moveTo(Point target, const Motion& motion = {});

    /// Moves by a number of pixels from the current position.
    Status moveBy(int dx, int dy, const Motion& motion = {});

    /// Relative movement in mickeys, the raw units games read for camera
    /// control. The distance in pixels depends on the pointer speed and
    /// acceleration settings, so the cursor position is not verified.
    Status moveRaw(int dx, int dy, const Motion& motion = {});

    Status down(MouseButton button = MouseButton::Left);
    Status up(MouseButton button = MouseButton::Left);

    /// Presses the button and returns a Hold that releases it.
    Hold hold(MouseButton button = MouseButton::Left);

    /// Clicks the button. Without hold or interval, all the clicks go in a
    /// single batch. If the system rejects part of it, the button is still released.
    Status click(MouseButton button = MouseButton::Left, const ClickOptions& options = {});

    /// Two clicks Windows recognizes as a double click. An `interval` equal to
    /// or longer than the system double-click time fails with InvalidArgument.
    Status doubleClick(MouseButton button = MouseButton::Left, std::chrono::milliseconds interval = {});

    /// Moves to `from`, presses the button, moves to `to` with `motion` and
    /// always releases the button, even if the movement failed.
    Status drag(Point from, Point to, const Motion& motion = Motion::smooth(std::chrono::milliseconds(300)),
                MouseButton button = MouseButton::Left);

    /// Wheel notches: positive scrolls up (away from the user), negative down.
    /// Fractions are supported (high-resolution wheels). With a duration the
    /// notches are spread evenly over that time.
    Status scroll(double notches, std::chrono::milliseconds duration = {});

    /// Horizontal wheel notches: positive scrolls right, negative left.
    Status scrollHorizontal(double notches, std::chrono::milliseconds duration = {});

    /// Whether this library currently holds the button down.
    bool isHeld(MouseButton button) const;

private:
    friend class Input;

    Mouse(detail::Session* session, bool verifyCursor) noexcept
        : session_(session), verifyCursor_(verifyCursor) {}

    // Used by Input's move operations: takes the session and leaves `other` empty.
    Mouse(Mouse&& other) noexcept
        : session_(std::exchange(other.session_, nullptr)), verifyCursor_(other.verifyCursor_) {}
    Mouse& operator=(Mouse&& other) noexcept {
        session_ = std::exchange(other.session_, nullptr);
        verifyCursor_ = other.verifyCursor_;
        return *this;
    }

    Status moveAbsolute(Point from, Point target, const Motion& motion);
    Status scrollAxis(double notches, std::chrono::milliseconds duration, bool horizontal);

    detail::Session* session_; // owned by Input; null after the Input was moved from
    bool verifyCursor_;
};

} // namespace inpututil
