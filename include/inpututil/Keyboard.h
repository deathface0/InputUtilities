#pragma once

#include <chrono>
#include <string_view>

#include "inpututil/Hold.h"
#include "inpututil/Key.h"
#include "inpututil/Status.h"

namespace inpututil {

namespace detail {
class Session;
}

/// Keyboard of an Input. Keys are injected according to mode() (see KeyMode).
///
///     input.keyboard.tap(Key::Enter);
///     input.keyboard.press("Ctrl+Shift+Esc");      // one atomic batch
///     auto shift = input.keyboard.hold(Key::Shift); // released with `shift`
class Keyboard {
public:
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    Status down(Key key);
    Status up(Key key);

    /// Presses and releases the key. Without `hold` both events go in a single
    /// batch; with it, the key stays down for that long.
    Status tap(Key key, std::chrono::milliseconds hold = {});

    /// Presses the keys in order and releases them in reverse order. Without
    /// `hold` everything goes in a single batch. If the system rejects part of
    /// it, every key is still released and the error is returned.
    Status press(const KeyCombo& combo, std::chrono::milliseconds hold = {});
    Status press(std::string_view combo, std::chrono::milliseconds hold = {});

    /// Presses the keys and returns a Hold that releases them (in reverse order).
    Hold hold(Key key);
    Hold hold(const KeyCombo& combo);
    Hold hold(std::string_view combo);

    /// Whether this library currently holds the key down.
    bool isHeld(Key key) const;

    KeyMode mode() const noexcept { return mode_; }
    void setMode(KeyMode mode) noexcept { mode_ = mode; }

private:
    friend class Input;

    Keyboard(detail::Session* session, KeyMode mode) noexcept : session_(session), mode_(mode) {}

    detail::Session* session_; // owned by Input; null after the Input was moved from
    KeyMode mode_;
};

} // namespace inpututil
