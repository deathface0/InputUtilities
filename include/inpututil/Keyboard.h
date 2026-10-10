#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>
#include <utility>

#include "inpututil/Hold.h"
#include "inpututil/Key.h"
#include "inpututil/Status.h"

namespace inpututil {

namespace detail {
class Session;
}

/// How Keyboard::type() produces the characters.
enum class TextMode : std::uint8_t {
    Unicode,    ///< KEYEVENTF_UNICODE: any character, independent of the keyboard layout.
    Keystrokes, ///< The real keys of the active layout (with Shift/AltGr), for apps that ignore
                ///< Unicode. See TypeOptions::fallbackToUnicode for characters without a single key.
};

struct TypeOptions {
    TextMode mode = TextMode::Unicode;

    /// Pause between characters.
    std::chrono::milliseconds delay{0};

    /// Random variation added to each pause, uniform in [-jitter, +jitter]
    /// (the pause never goes below zero).
    std::chrono::milliseconds jitter{0};

    /// Keystrokes mode: send characters without a single key (no key, a dead
    /// key such as ^ or ´, or an emoji) as Unicode. When false, such a
    /// character fails the whole call with UnmappableCharacter.
    bool fallbackToUnicode = true;

    /// Seed of the jitter; 0 picks a random one, any other value is reproducible.
    std::uint32_t seed = 0;
};

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
    /// it, every key that went down is released and the error is returned.
    Status press(const KeyCombo& combo, std::chrono::milliseconds hold = {});
    Status press(std::string_view combo, std::chrono::milliseconds hold = {});

    /// Presses the keys and returns a Hold that releases them (in reverse order).
    Hold hold(Key key);
    Hold hold(const KeyCombo& combo);
    Hold hold(std::string_view combo);

    /// Types text, one character per batch. "\n", "\r\n" and "\r" press Enter,
    /// "\t" presses Tab. Invalid text (bad UTF-8, unpaired surrogates, other
    /// control characters) fails with InvalidArgument before anything is sent.
    Status type(std::string_view utf8, const TypeOptions& options = {});
    Status type(std::wstring_view text, const TypeOptions& options = {});
    Status type(std::u8string_view text, const TypeOptions& options = {});

    /// Whether this library currently holds the key down.
    bool isHeld(Key key) const;

    KeyMode mode() const noexcept { return mode_; }
    void setMode(KeyMode mode) noexcept { mode_ = mode; }

private:
    friend class Input;

    Keyboard(detail::Session* session, KeyMode mode) noexcept : session_(session), mode_(mode) {}

    // Used by Input's move operations: takes the session and leaves `other` empty.
    Keyboard(Keyboard&& other) noexcept
        : session_(std::exchange(other.session_, nullptr)), mode_(other.mode_) {}
    Keyboard& operator=(Keyboard&& other) noexcept {
        session_ = std::exchange(other.session_, nullptr);
        mode_ = other.mode_;
        return *this;
    }

    detail::Session* session_; // owned by Input; null after the Input was moved from
    KeyMode mode_;
};

} // namespace inpututil
