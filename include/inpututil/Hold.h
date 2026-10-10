#pragma once

#include <functional>

#include "inpututil/Status.h"

namespace inpututil {

/// Keeps keys or mouse buttons pressed while it lives and releases them when
/// destroyed, moved over or release() is called (RAII):
///
///     {
///         auto shift = input.keyboard.hold(Key::Shift);
///         input.keyboard.tap(Key::Right);
///     } // Shift released here
///
/// Safe to outlive its Input: once the Input is gone, release() does nothing.
class [[nodiscard]] Hold {
public:
    Hold() = default; ///< Empty: releases nothing.

    Hold(Hold&& other) noexcept;
    Hold& operator=(Hold&& other) noexcept;
    Hold(const Hold&) = delete;
    Hold& operator=(const Hold&) = delete;
    ~Hold();

    /// Releases now. Calling it again does nothing and returns success.
    Status release();

    /// Whether release() still has something to do. The keys may already be up
    /// if releaseAll() or the abort key released them; release() then sends nothing.
    bool active() const noexcept { return static_cast<bool>(release_); }

    /// Result of the initial press; when it failed, nothing is held.
    Status status() const noexcept { return status_; }

private:
    friend class Keyboard;
    friend class Mouse;

    Hold(std::function<Status()> release, Status status) noexcept;
    void releaseQuietly() noexcept;

    std::function<Status()> release_;
    Status status_;
};

} // namespace inpututil
