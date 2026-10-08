#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

#include "inpututil/Point.h"

// The Win32 INPUT structure (typedef struct tagINPUT INPUT). Forward declared
// so public headers do not drag <windows.h> into user code.
struct tagINPUT;

namespace inpututil {

/// Everything inpututil needs from the operating system. The library uses
/// win32Backend() by default; tests inject a fake, and advanced users can plug
/// in their own implementation (e.g. a kernel-driver based injector).
class Backend {
public:
    using Clock = std::chrono::steady_clock;

    virtual ~Backend() = default;

    // --- Injection -------------------------------------------------------

    /// Injects the events in order. Returns how many were accepted, like
    /// SendInput (fewer than inputs.size() means the rest were rejected).
    virtual unsigned sendInput(std::span<const tagINPUT> inputs) = 0;

    /// GetLastError() right after a failed call.
    virtual std::uint32_t lastError() = 0;

    // --- Screen and cursor -----------------------------------------------

    /// Current cursor position, or nullopt if it cannot be queried.
    virtual std::optional<Point> cursorPos() = 0;

    /// Bounds of the virtual desktop (all monitors).
    virtual Rect virtualScreen() = 0;

    // --- Keyboard layout of the foreground window ------------------------

    /// VkKeyScanExW semantics: low byte = virtual key, high byte = required
    /// modifiers (1 Shift, 2 Ctrl, 4 Alt). -1 if the character has no key.
    virtual std::int16_t vkKeyScan(wchar_t ch) = 0;

    /// MAPVK_VK_TO_VSC_EX semantics: scan code with 0xE0/0xE1 in the high
    /// byte for extended keys, 0 if the key has no scan code.
    virtual std::uint16_t vkToScanCode(std::uint16_t vk) = 0;

    /// MAPVK_VSC_TO_VK_EX semantics (accepts the 0xE0 prefix), 0 if unknown.
    virtual std::uint16_t scanCodeToVk(std::uint16_t scanCode) = 0;

    // --- Key state -------------------------------------------------------

    /// Whether the key is currently down (GetAsyncKeyState).
    virtual bool isKeyDown(std::uint16_t vk) = 0;

    /// Whether a toggle key such as Caps Lock is on (GetKeyState & 1).
    virtual bool isKeyToggled(std::uint16_t vk) = 0;

    // --- Time ------------------------------------------------------------

    virtual Clock::time_point now() = 0;

    /// Blocks until the deadline (returns immediately if it already passed).
    virtual void sleepUntil(Clock::time_point deadline) = 0;

    /// Maximum interval between the clicks of a double click.
    virtual std::chrono::milliseconds doubleClickTime() = 0;
};

/// Process-wide instance of the real Win32 backend.
std::shared_ptr<Backend> win32Backend();

} // namespace inpututil
