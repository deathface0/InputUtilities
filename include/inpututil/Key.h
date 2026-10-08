#pragma once

#include <cstdint>

namespace inpututil {

/// How key events are injected.
enum class KeyMode : std::uint8_t {
    VirtualKey, ///< Virtual key + scan code + extended flag: the most compatible with applications.
    ScanCode,   ///< KEYEVENTF_SCANCODE: what most DirectInput / raw input games need.
};

/// A keyboard key, identified either by virtual key (what the key means in the
/// active layout) or by scan code (its physical position, e.g. WASD on any
/// layout). How it is injected is decided by KeyMode, not by the Key.
class Key {
public:
    constexpr Key() noexcept = default; ///< Not a valid key.

    static constexpr Key fromVk(std::uint16_t vk) noexcept { return {Kind::Vk, vk, false}; }

    /// Physical key. Pass extended = true for keys sent with the 0xE0 prefix
    /// (right Ctrl/Alt, arrows, Insert, Delete, Home, End, PageUp/Down...).
    static constexpr Key fromScanCode(std::uint16_t scanCode, bool extended = false) noexcept {
        return {Kind::Scan, static_cast<std::uint16_t>(scanCode & 0xFF),
                extended || (scanCode & 0xFF00) == 0xE000};
    }

    constexpr bool valid() const noexcept { return kind_ != Kind::None && code_ != 0; }
    constexpr bool isScanCode() const noexcept { return kind_ == Kind::Scan; }

    /// Virtual key, 0 for keys created from a scan code.
    constexpr std::uint16_t vk() const noexcept { return kind_ == Kind::Vk ? code_ : 0; }

    /// Scan code, 0 for keys created from a virtual key.
    constexpr std::uint16_t scanCode() const noexcept { return kind_ == Kind::Scan ? code_ : 0; }

    /// Extended flag of a scan-code key (virtual keys are resolved when sent).
    constexpr bool extended() const noexcept { return extended_; }

    friend constexpr bool operator==(Key, Key) noexcept = default;

    // Letters and digits
    static const Key A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z;
    static const Key Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9;

    // Numeric keypad
    static const Key Numpad0, Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9;
    static const Key NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide, NumpadDecimal, NumpadEnter;

    // Function keys
    static const Key F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12;
    static const Key F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24;

    // Modifiers
    static const Key Shift, LShift, RShift, Ctrl, LCtrl, RCtrl, Alt, LAlt, RAlt, AltGr, Win, LWin, RWin, Apps;

    // Editing and navigation
    static const Key Enter, Esc, Tab, Space, Backspace, CapsLock, NumLock, ScrollLock, PrintScreen, Pause;
    static const Key Insert, Delete, Home, End, PageUp, PageDown, Left, Up, Right, Down;

    // Punctuation keys whose meaning is the same in most layouts
    static const Key OemPlus, OemMinus, OemComma, OemPeriod;

    // Media
    static const Key VolumeMute, VolumeDown, VolumeUp, MediaNext, MediaPrev, MediaStop, MediaPlayPause;

private:
    enum class Kind : std::uint8_t { None, Vk, Scan };

    constexpr Key(Kind kind, std::uint16_t code, bool extended) noexcept
        : kind_(kind), code_(code), extended_(extended) {}

    Kind kind_ = Kind::None;
    std::uint16_t code_ = 0;
    bool extended_ = false;
};

// Values are the Win32 VK_* codes (written as numbers so this header does not
// need <windows.h>).
inline constexpr Key Key::A = Key::fromVk(0x41);
inline constexpr Key Key::B = Key::fromVk(0x42);
inline constexpr Key Key::C = Key::fromVk(0x43);
inline constexpr Key Key::D = Key::fromVk(0x44);
inline constexpr Key Key::E = Key::fromVk(0x45);
inline constexpr Key Key::F = Key::fromVk(0x46);
inline constexpr Key Key::G = Key::fromVk(0x47);
inline constexpr Key Key::H = Key::fromVk(0x48);
inline constexpr Key Key::I = Key::fromVk(0x49);
inline constexpr Key Key::J = Key::fromVk(0x4A);
inline constexpr Key Key::K = Key::fromVk(0x4B);
inline constexpr Key Key::L = Key::fromVk(0x4C);
inline constexpr Key Key::M = Key::fromVk(0x4D);
inline constexpr Key Key::N = Key::fromVk(0x4E);
inline constexpr Key Key::O = Key::fromVk(0x4F);
inline constexpr Key Key::P = Key::fromVk(0x50);
inline constexpr Key Key::Q = Key::fromVk(0x51);
inline constexpr Key Key::R = Key::fromVk(0x52);
inline constexpr Key Key::S = Key::fromVk(0x53);
inline constexpr Key Key::T = Key::fromVk(0x54);
inline constexpr Key Key::U = Key::fromVk(0x55);
inline constexpr Key Key::V = Key::fromVk(0x56);
inline constexpr Key Key::W = Key::fromVk(0x57);
inline constexpr Key Key::X = Key::fromVk(0x58);
inline constexpr Key Key::Y = Key::fromVk(0x59);
inline constexpr Key Key::Z = Key::fromVk(0x5A);

inline constexpr Key Key::Digit0 = Key::fromVk(0x30);
inline constexpr Key Key::Digit1 = Key::fromVk(0x31);
inline constexpr Key Key::Digit2 = Key::fromVk(0x32);
inline constexpr Key Key::Digit3 = Key::fromVk(0x33);
inline constexpr Key Key::Digit4 = Key::fromVk(0x34);
inline constexpr Key Key::Digit5 = Key::fromVk(0x35);
inline constexpr Key Key::Digit6 = Key::fromVk(0x36);
inline constexpr Key Key::Digit7 = Key::fromVk(0x37);
inline constexpr Key Key::Digit8 = Key::fromVk(0x38);
inline constexpr Key Key::Digit9 = Key::fromVk(0x39);

inline constexpr Key Key::Numpad0 = Key::fromVk(0x60);
inline constexpr Key Key::Numpad1 = Key::fromVk(0x61);
inline constexpr Key Key::Numpad2 = Key::fromVk(0x62);
inline constexpr Key Key::Numpad3 = Key::fromVk(0x63);
inline constexpr Key Key::Numpad4 = Key::fromVk(0x64);
inline constexpr Key Key::Numpad5 = Key::fromVk(0x65);
inline constexpr Key Key::Numpad6 = Key::fromVk(0x66);
inline constexpr Key Key::Numpad7 = Key::fromVk(0x67);
inline constexpr Key Key::Numpad8 = Key::fromVk(0x68);
inline constexpr Key Key::Numpad9 = Key::fromVk(0x69);
inline constexpr Key Key::NumpadMultiply = Key::fromVk(0x6A);
inline constexpr Key Key::NumpadAdd = Key::fromVk(0x6B);
inline constexpr Key Key::NumpadSubtract = Key::fromVk(0x6D);
inline constexpr Key Key::NumpadDecimal = Key::fromVk(0x6E);
inline constexpr Key Key::NumpadDivide = Key::fromVk(0x6F);
inline constexpr Key Key::NumpadEnter = Key::fromScanCode(0x1C, true); // shares VK_RETURN with Enter

inline constexpr Key Key::F1 = Key::fromVk(0x70);
inline constexpr Key Key::F2 = Key::fromVk(0x71);
inline constexpr Key Key::F3 = Key::fromVk(0x72);
inline constexpr Key Key::F4 = Key::fromVk(0x73);
inline constexpr Key Key::F5 = Key::fromVk(0x74);
inline constexpr Key Key::F6 = Key::fromVk(0x75);
inline constexpr Key Key::F7 = Key::fromVk(0x76);
inline constexpr Key Key::F8 = Key::fromVk(0x77);
inline constexpr Key Key::F9 = Key::fromVk(0x78);
inline constexpr Key Key::F10 = Key::fromVk(0x79);
inline constexpr Key Key::F11 = Key::fromVk(0x7A);
inline constexpr Key Key::F12 = Key::fromVk(0x7B);
inline constexpr Key Key::F13 = Key::fromVk(0x7C);
inline constexpr Key Key::F14 = Key::fromVk(0x7D);
inline constexpr Key Key::F15 = Key::fromVk(0x7E);
inline constexpr Key Key::F16 = Key::fromVk(0x7F);
inline constexpr Key Key::F17 = Key::fromVk(0x80);
inline constexpr Key Key::F18 = Key::fromVk(0x81);
inline constexpr Key Key::F19 = Key::fromVk(0x82);
inline constexpr Key Key::F20 = Key::fromVk(0x83);
inline constexpr Key Key::F21 = Key::fromVk(0x84);
inline constexpr Key Key::F22 = Key::fromVk(0x85);
inline constexpr Key Key::F23 = Key::fromVk(0x86);
inline constexpr Key Key::F24 = Key::fromVk(0x87);

inline constexpr Key Key::Shift = Key::fromVk(0x10);
inline constexpr Key Key::LShift = Key::fromVk(0xA0);
inline constexpr Key Key::RShift = Key::fromVk(0xA1);
inline constexpr Key Key::Ctrl = Key::fromVk(0x11);
inline constexpr Key Key::LCtrl = Key::fromVk(0xA2);
inline constexpr Key Key::RCtrl = Key::fromVk(0xA3);
inline constexpr Key Key::Alt = Key::fromVk(0x12);
inline constexpr Key Key::LAlt = Key::fromVk(0xA4);
inline constexpr Key Key::RAlt = Key::fromVk(0xA5);
inline constexpr Key Key::AltGr = Key::RAlt;
inline constexpr Key Key::LWin = Key::fromVk(0x5B);
inline constexpr Key Key::RWin = Key::fromVk(0x5C);
inline constexpr Key Key::Win = Key::LWin;
inline constexpr Key Key::Apps = Key::fromVk(0x5D);

inline constexpr Key Key::Enter = Key::fromVk(0x0D);
inline constexpr Key Key::Esc = Key::fromVk(0x1B);
inline constexpr Key Key::Tab = Key::fromVk(0x09);
inline constexpr Key Key::Space = Key::fromVk(0x20);
inline constexpr Key Key::Backspace = Key::fromVk(0x08);
inline constexpr Key Key::CapsLock = Key::fromVk(0x14);
inline constexpr Key Key::NumLock = Key::fromVk(0x90);
inline constexpr Key Key::ScrollLock = Key::fromVk(0x91);
inline constexpr Key Key::PrintScreen = Key::fromVk(0x2C);
inline constexpr Key Key::Pause = Key::fromVk(0x13);
inline constexpr Key Key::Insert = Key::fromVk(0x2D);
inline constexpr Key Key::Delete = Key::fromVk(0x2E);
inline constexpr Key Key::Home = Key::fromVk(0x24);
inline constexpr Key Key::End = Key::fromVk(0x23);
inline constexpr Key Key::PageUp = Key::fromVk(0x21);
inline constexpr Key Key::PageDown = Key::fromVk(0x22);
inline constexpr Key Key::Left = Key::fromVk(0x25);
inline constexpr Key Key::Up = Key::fromVk(0x26);
inline constexpr Key Key::Right = Key::fromVk(0x27);
inline constexpr Key Key::Down = Key::fromVk(0x28);

inline constexpr Key Key::OemPlus = Key::fromVk(0xBB);
inline constexpr Key Key::OemComma = Key::fromVk(0xBC);
inline constexpr Key Key::OemMinus = Key::fromVk(0xBD);
inline constexpr Key Key::OemPeriod = Key::fromVk(0xBE);

inline constexpr Key Key::VolumeMute = Key::fromVk(0xAD);
inline constexpr Key Key::VolumeDown = Key::fromVk(0xAE);
inline constexpr Key Key::VolumeUp = Key::fromVk(0xAF);
inline constexpr Key Key::MediaNext = Key::fromVk(0xB0);
inline constexpr Key Key::MediaPrev = Key::fromVk(0xB1);
inline constexpr Key Key::MediaStop = Key::fromVk(0xB2);
inline constexpr Key Key::MediaPlayPause = Key::fromVk(0xB3);

} // namespace inpututil
