#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "inpututil/KeyList.h"

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

    /// Key from its name: "Ctrl", "f5", "PgUp", "AltGr", "+", ... (case
    /// insensitive), or a raw code: "vk:0x41", "sc:0x11", "sc:0xE04B".
    static std::optional<Key> parse(std::string_view name);

    /// Name of the matching Key constant ("Ctrl", "F5", "Digit7", "RAlt"), or
    /// the raw form ("vk:0xE9", "sc:0x76") for keys without one.
    /// Key::parse(k.name()) == k always holds.
    std::string name() const;

    constexpr bool valid() const noexcept { return kind_ != Kind::None && code_ != 0; }
    constexpr bool isScanCode() const noexcept { return kind_ == Kind::Scan; }

    /// Virtual key, 0 for keys created from a scan code.
    constexpr std::uint16_t vk() const noexcept { return kind_ == Kind::Vk ? code_ : 0; }

    /// Scan code, 0 for keys created from a virtual key.
    constexpr std::uint16_t scanCode() const noexcept { return kind_ == Kind::Scan ? code_ : 0; }

    /// Extended flag of a scan-code key (virtual keys are resolved when sent).
    constexpr bool extended() const noexcept { return extended_; }

    friend constexpr bool operator==(Key, Key) noexcept = default;

    // Named keys: Key::A, Key::F5, Key::Ctrl, Key::Left... (see KeyList.h)
#define INPUTUTIL_DECLARE_KEY(name, expr) static const Key name;
    INPUTUTIL_NAMED_KEYS(INPUTUTIL_DECLARE_KEY)
#undef INPUTUTIL_DECLARE_KEY

private:
    enum class Kind : std::uint8_t { None, Vk, Scan };

    constexpr Key(Kind kind, std::uint16_t code, bool extended) noexcept
        : kind_(kind), code_(code), extended_(extended) {}

    Kind kind_ = Kind::None;
    std::uint16_t code_ = 0;
    bool extended_ = false;
};

#define INPUTUTIL_DEFINE_KEY(name, expr) inline constexpr Key Key::name = Key::expr;
INPUTUTIL_NAMED_KEYS(INPUTUTIL_DEFINE_KEY)
#undef INPUTUTIL_DEFINE_KEY

/// Keys pressed together, in press order (released in reverse).
struct Chord {
    std::vector<Key> keys;

    /// "Ctrl+Shift+Esc", " alt + f4 ", "Ctrl++" (Ctrl and Plus). nullopt if a
    /// name is unknown, a key repeats or the text is empty or ends with '+'.
    static std::optional<Chord> parse(std::string_view text);

    /// Key names joined with '+'; Chord::parse(c.toString()) == c.
    std::string toString() const;

    friend bool operator==(const Chord&, const Chord&) = default;
};

} // namespace inpututil
