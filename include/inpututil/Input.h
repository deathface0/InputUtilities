#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

#include "inpututil/Backend.h"
#include "inpututil/Key.h"
#include "inpututil/Keyboard.h"
#include "inpututil/Mouse.h"
#include "inpututil/Sequence.h"
#include "inpututil/Status.h"

namespace inpututil {

namespace detail {
class Session;
}

/// Default dwExtraInfo stamped on every injected event ("IUTI"), so hooks can
/// tell inpututil's input apart from real input.
inline constexpr std::uintptr_t kDefaultExtraInfoTag = 0x49555449;

struct Config {
    /// How keys are injected; can be changed later with keyboard.setMode().
    KeyMode keyMode = KeyMode::VirtualKey;

    /// Release every key and button still held when the Input is destroyed.
    bool releaseOnDestroy = true;

    /// After mouse.moveTo/moveBy, check that the cursor reached the target
    /// (TargetNotReached otherwise).
    bool verifyCursor = false;

    /// Key that stops long operations (motions, holds, typing, timed clicks,
    /// sequence waits): they release everything and return Error::Aborted.
    std::optional<Key> abortKey{};

    /// dwExtraInfo of the injected events.
    std::uintptr_t extraInfoTag = kDefaultExtraInfoTag;

    /// System access; nullptr means win32Backend().
    std::shared_ptr<Backend> backend{};
};

/// Entry point of the library. Tracks everything it presses so it can always
/// be released (releaseAll(), or automatically on destruction).
///
///     inpututil::Input input;
///     input.keyboard.press("Ctrl+C");
///
///     inpututil::Input game({.keyMode = inpututil::KeyMode::ScanCode});
class Input {
private:
    // Declared first: the members below are initialized from it.
    std::shared_ptr<detail::Session> session_;
    bool releaseOnDestroy_;

public:
    explicit Input(Config config = {});
    ~Input();

    /// The moved-from Input is left empty: its operations return InvalidArgument.
    Input(Input&& other) noexcept;
    Input& operator=(Input&& other) noexcept;
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    Keyboard keyboard;
    Mouse mouse;

    /// Releases every key and button currently held, last pressed first.
    Status releaseAll();

    /// Plays a Sequence. Every step is validated first: a bad step fails the
    /// call before anything is sent. If a step fails midway, whatever the
    /// sequence pressed is released and the error is returned.
    Status play(const Sequence& sequence);

    /// Sends raw Win32 INPUT events (include <windows.h> to build them) in one
    /// batch. Keys and buttons pressed this way are tracked like any other, so
    /// releaseAll() and the destructor release them too.
    Status sendRaw(std::span<const tagINPUT> events);

    /// Number of keys and buttons currently held.
    std::size_t heldCount() const;

    /// Changes or removes the abort key; safe to call from any thread.
    void setAbortKey(std::optional<Key> key);
};

} // namespace inpututil
