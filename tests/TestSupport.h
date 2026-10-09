#pragma once

// Common include for every test: doctest plus the helpers shared by the
// test files. <ostream> must be visible so doctest can print std types.
#include <windows.h>

#include <ostream>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

/// Whether a keyboard event is a release.
inline bool isKeyUp(const INPUT& in) { return (in.ki.dwFlags & KEYEVENTF_KEYUP) != 0; }

/// Whether the event presses (up = false) or releases (up = true) the virtual key.
inline bool isKeyEvent(const INPUT& in, WORD vk, bool up) {
    return in.type == INPUT_KEYBOARD && in.ki.wVk == vk && isKeyUp(in) == up;
}
