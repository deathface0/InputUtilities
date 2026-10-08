#pragma once

#include <cstdint>
#include <optional>

#include "inpututil/Hold.h"
#include "inpututil/Motion.h"
#include "inpututil/Point.h"
#include "inpututil/Status.h"

namespace inpututil {

namespace detail {
class Session;
}

enum class MouseButton : std::uint8_t { Left, Right, Middle, X1, X2 };

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

    /// Whether this library currently holds the button down.
    bool isHeld(MouseButton button) const;

private:
    friend class Input;

    Mouse(detail::Session* session, bool verifyCursor) noexcept : session_(session), verifyCursor_(verifyCursor) {}

    Status moveAbsolute(Point from, Point target, const Motion& motion);

    detail::Session* session_; // owned by Input; null after the Input was moved from
    bool verifyCursor_;
};

} // namespace inpututil
