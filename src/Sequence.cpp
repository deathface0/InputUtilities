#include "inpututil/Sequence.h"

#include "detail/Text.h"

namespace inpututil {

namespace steps = detail::steps;

Sequence& Sequence::down(Key key) {
    steps_.emplace_back(steps::KeyDown{key});
    return *this;
}

Sequence& Sequence::up(Key key) {
    steps_.emplace_back(steps::KeyUp{key});
    return *this;
}

Sequence& Sequence::tap(Key key, std::chrono::milliseconds hold) { return press(KeyCombo{{key}}, hold); }

Sequence& Sequence::press(const KeyCombo& combo, std::chrono::milliseconds hold) {
    steps_.emplace_back(steps::Press{combo, hold});
    return *this;
}

Sequence& Sequence::press(std::string_view combo, std::chrono::milliseconds hold) {
    // Parsed now so that play() can reject the whole sequence before sending anything.
    if (const auto parsed = KeyCombo::parse(combo)) return press(*parsed, hold);
    steps_.emplace_back(steps::Invalid{});
    return *this;
}

Sequence& Sequence::type(std::string_view utf8, const TypeOptions& options) {
    if (const auto text = detail::utf8ToUtf16(utf8)) return type(std::wstring_view(*text), options);
    steps_.emplace_back(steps::Invalid{});
    return *this;
}

Sequence& Sequence::type(std::wstring_view text, const TypeOptions& options) {
    steps_.emplace_back(steps::Type{std::wstring(text), options});
    return *this;
}

Sequence& Sequence::mouseDown(MouseButton button) {
    steps_.emplace_back(steps::ButtonDown{button});
    return *this;
}

Sequence& Sequence::mouseUp(MouseButton button) {
    steps_.emplace_back(steps::ButtonUp{button});
    return *this;
}

Sequence& Sequence::click(MouseButton button, const ClickOptions& options) {
    steps_.emplace_back(steps::Click{button, options});
    return *this;
}

Sequence& Sequence::moveTo(Point target, const Motion& motion) {
    steps_.emplace_back(steps::MoveTo{target, motion});
    return *this;
}

Sequence& Sequence::moveBy(int dx, int dy, const Motion& motion) {
    steps_.emplace_back(steps::MoveBy{dx, dy, motion});
    return *this;
}

Sequence& Sequence::moveRaw(int dx, int dy, const Motion& motion) {
    steps_.emplace_back(steps::MoveRaw{dx, dy, motion});
    return *this;
}

Sequence& Sequence::scroll(double notches, std::chrono::milliseconds duration) {
    steps_.emplace_back(steps::Scroll{notches, duration, false});
    return *this;
}

Sequence& Sequence::scrollHorizontal(double notches, std::chrono::milliseconds duration) {
    steps_.emplace_back(steps::Scroll{notches, duration, true});
    return *this;
}

Sequence& Sequence::wait(std::chrono::milliseconds duration) {
    steps_.emplace_back(steps::Wait{duration});
    return *this;
}

} // namespace inpututil
