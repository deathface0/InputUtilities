#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace inpututil {

enum class Error : std::uint8_t {
    None,                ///< The operation succeeded.
    SystemFailure,       ///< SendInput (or another Win32 call) failed; see Status::win32Error().
    PartialSend,         ///< Only part of a batch was injected; the rest was rejected by the system.
    InvalidArgument,     ///< A parameter was out of range, unparsable or the object was moved-from.
    UnmappableCharacter, ///< A character has no key in the active keyboard layout.
    TargetNotReached,    ///< The cursor did not end up where requested (Config::verifyCursor).
    Aborted,             ///< The abort key was pressed; every held input was released.
};

/// Human readable name of an error code, e.g. "PartialSend".
std::string_view errorName(Error error) noexcept;

/// Result of every inpututil operation. Converts to true on success:
///
///     if (auto st = input.keyboard.press("Ctrl+C"); !st)
///         std::cerr << st.message() << '\n';
class Status {
public:
    constexpr Status() noexcept = default;
    constexpr Status(Error error, std::uint32_t win32Error = 0) noexcept // NOLINT: implicit by design
        : error_(error), win32Error_(win32Error) {}

    constexpr bool ok() const noexcept { return error_ == Error::None; }
    constexpr explicit operator bool() const noexcept { return ok(); }

    constexpr Error error() const noexcept { return error_; }

    /// GetLastError() captured when the failure happened, 0 when not applicable.
    constexpr std::uint32_t win32Error() const noexcept { return win32Error_; }

    /// Error name plus the system message for win32Error(), if any.
    std::string message() const;

    friend constexpr bool operator==(const Status& a, const Status& b) noexcept = default;
    friend constexpr bool operator==(const Status& a, Error e) noexcept { return a.error_ == e; }

private:
    Error error_ = Error::None;
    std::uint32_t win32Error_ = 0;
};

/// A Status plus how far an operation made of parts got: the characters of
/// Keyboard::type() or the steps of Input::play().
///
///     if (Progress p = input.play(macro); !p && p.failedAt())
///         std::printf("step %zu failed: %s\n", *p.failedAt(), p.message().c_str());
class Progress : public Status {
public:
    constexpr Progress(Status status = {}, std::size_t completed = 0, // NOLINT: implicit by design
                       std::optional<std::size_t> failedAt = std::nullopt) noexcept
        : Status(status), completed_(completed), failedAt_(failedAt) {}
    constexpr Progress(Error error) noexcept : Progress(Status(error)) {} // NOLINT: implicit by design

    /// Parts sent completely; all of them on success.
    constexpr std::size_t completed() const noexcept { return completed_; }

    /// The part that made it fail: an invalid one found before anything was
    /// sent (then completed() is 0), or the one being sent or waited for when
    /// it failed or was aborted. Empty on success and for failures not tied to
    /// a part (a moved-from object, text that is not valid UTF-8).
    constexpr std::optional<std::size_t> failedAt() const noexcept { return failedAt_; }

private:
    std::size_t completed_ = 0;
    std::optional<std::size_t> failedAt_;
};

} // namespace inpututil
