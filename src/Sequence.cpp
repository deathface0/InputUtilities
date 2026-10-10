#include "inpututil/Sequence.h"

#include "detail/Text.h"

namespace inpututil {

namespace steps = detail::steps;

Sequence& Sequence::down(Key key) { return add(steps::KeyDown{key}); }

Sequence& Sequence::up(Key key) { return add(steps::KeyUp{key}); }

Sequence& Sequence::tap(Key key, std::chrono::milliseconds hold) { return press(KeyCombo{{key}}, hold); }

Sequence& Sequence::press(const KeyCombo& combo, std::chrono::milliseconds hold) {
    return add(steps::Press{combo, hold});
}

Sequence& Sequence::press(std::string_view combo, std::chrono::milliseconds hold) {
    // Parsed now so that play() can reject the whole sequence before sending anything.
    if (const auto parsed = KeyCombo::parse(combo)) return press(*parsed, hold);
    return add(steps::Invalid{});
}

Sequence& Sequence::type(std::string_view utf8, const TypeOptions& options) {
    if (const auto text = detail::utf8ToUtf16(utf8)) return type(std::wstring_view(*text), options);
    return add(steps::Invalid{});
}

Sequence& Sequence::type(std::wstring_view text, const TypeOptions& options) {
    return add(steps::Type{std::wstring(text), options});
}

Sequence& Sequence::mouseDown(MouseButton button) { return add(steps::ButtonDown{button}); }

Sequence& Sequence::mouseUp(MouseButton button) { return add(steps::ButtonUp{button}); }

Sequence& Sequence::click(MouseButton button, const ClickOptions& options) {
    return add(steps::Click{button, options});
}

Sequence& Sequence::moveTo(Point target, const Motion& motion) { return add(steps::MoveTo{target, motion}); }

Sequence& Sequence::moveBy(int dx, int dy, const Motion& motion) {
    return add(steps::MoveBy{dx, dy, motion});
}

Sequence& Sequence::moveRaw(int dx, int dy, const Motion& motion) {
    return add(steps::MoveRaw{dx, dy, motion});
}

Sequence& Sequence::scroll(double notches, std::chrono::milliseconds duration) {
    return add(steps::Scroll{notches, duration, false});
}

Sequence& Sequence::scrollHorizontal(double notches, std::chrono::milliseconds duration) {
    return add(steps::Scroll{notches, duration, true});
}

Sequence& Sequence::wait(std::chrono::milliseconds duration) { return add(steps::Wait{duration}); }

} // namespace inpututil
