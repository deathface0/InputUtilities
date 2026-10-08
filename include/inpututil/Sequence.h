#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "inpututil/Key.h"
#include "inpututil/Keyboard.h"
#include "inpututil/Motion.h"
#include "inpututil/Mouse.h"
#include "inpututil/Point.h"

namespace inpututil {

namespace detail {

// One recorded action of a Sequence.
namespace steps {
struct Invalid {}; // an argument that could not be parsed when it was added
struct KeyDown {
    Key key;
};
struct KeyUp {
    Key key;
};
struct Press {
    KeyCombo combo;
    std::chrono::milliseconds hold;
};
struct Type {
    std::wstring text;
    TypeOptions options;
};
struct ButtonDown {
    MouseButton button;
};
struct ButtonUp {
    MouseButton button;
};
struct Click {
    MouseButton button;
    ClickOptions options;
};
struct MoveTo {
    Point target;
    Motion motion;
};
struct MoveBy {
    int dx;
    int dy;
    Motion motion;
};
struct MoveRaw {
    int dx;
    int dy;
    Motion motion;
};
struct Scroll {
    double notches;
    std::chrono::milliseconds duration;
    bool horizontal;
};
struct Wait {
    std::chrono::milliseconds duration;
};
} // namespace steps

using SequenceStep =
    std::variant<steps::Invalid, steps::KeyDown, steps::KeyUp, steps::Press, steps::Type, steps::ButtonDown,
                 steps::ButtonUp, steps::Click, steps::MoveTo, steps::MoveBy, steps::MoveRaw, steps::Scroll,
                 steps::Wait>;

} // namespace detail

/// A macro as a value: build it once, copy it, play it as often as needed
/// with Input::play(). Consecutive instant actions are sent in one atomic
/// batch; the batch only breaks where time passes (waits, holds, motions,
/// typing) or where the current state is needed (moveBy).
///
///     const auto paste = Sequence{}.click().press("Ctrl+V").wait(100ms).type("done\n");
///     input.play(paste);
class Sequence {
public:
    // Keyboard
    Sequence& down(Key key);
    Sequence& up(Key key);
    Sequence& tap(Key key, std::chrono::milliseconds hold = {});
    Sequence& press(const KeyCombo& combo, std::chrono::milliseconds hold = {});
    Sequence& press(std::string_view combo, std::chrono::milliseconds hold = {});
    Sequence& type(std::string_view utf8, const TypeOptions& options = {});
    Sequence& type(std::wstring_view text, const TypeOptions& options = {});

    // Mouse
    Sequence& mouseDown(MouseButton button = MouseButton::Left);
    Sequence& mouseUp(MouseButton button = MouseButton::Left);
    Sequence& click(MouseButton button = MouseButton::Left, const ClickOptions& options = {});
    Sequence& moveTo(Point target, const Motion& motion = {});
    Sequence& moveBy(int dx, int dy, const Motion& motion = {});
    Sequence& moveRaw(int dx, int dy, const Motion& motion = {});
    Sequence& scroll(double notches, std::chrono::milliseconds duration = {});
    Sequence& scrollHorizontal(double notches, std::chrono::milliseconds duration = {});

    // Time
    Sequence& wait(std::chrono::milliseconds duration);

    std::size_t size() const noexcept { return steps_.size(); }
    bool empty() const noexcept { return steps_.empty(); }

private:
    friend class Input;

    std::vector<detail::SequenceStep> steps_;
};

} // namespace inpututil
