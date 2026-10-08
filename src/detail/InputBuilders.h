#pragma once

#include <windows.h>

#include <cstdint>
#include <optional>

#include "inpututil/Backend.h"
#include "inpututil/Key.h"
#include "inpututil/Mouse.h"
#include "inpututil/Point.h"

// Builders that turn the public value types into Win32 INPUT events.
namespace inpututil::detail {

/// Whether Windows expects KEYEVENTF_EXTENDEDKEY for this virtual key.
/// MapVirtualKeyEx does not report it for arrows, Insert, Delete, Home, End,
/// PageUp/Down, NumLock or PrintScreen, so the list is kept here.
bool isExtendedVk(std::uint16_t vk);

struct ResolvedKey {
    std::uint16_t vk = 0;   ///< 0 if the key has no virtual key in the layout
    std::uint16_t scan = 0; ///< scan code without prefix, 0 if unknown
    bool extended = false;  ///< needs KEYEVENTF_EXTENDEDKEY
    bool scanUsable = true; ///< false for keys that cannot be sent as one scan code (Pause)
};

/// Completes a Key with the information the other mode needs (vk <-> scan code).
std::optional<ResolvedKey> resolveKey(const Key& key, Backend& backend);

/// Key event for the given mode, falling back to the other mode when the key
/// cannot be expressed in the requested one. nullopt for an invalid key.
std::optional<INPUT> makeKeyInput(const Key& key, KeyMode mode, bool up, Backend& backend);

/// Absolute move to a pixel of the virtual desktop (clamped to it):
/// MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK.
INPUT makeAbsoluteMove(Point target, const Rect& screen);

/// Relative move in mickeys (subject to the user's pointer speed and acceleration).
INPUT makeRelativeMove(int dx, int dy);

/// Press or release of a mouse button.
INPUT makeButtonInput(MouseButton button, bool up);

} // namespace inpututil::detail
