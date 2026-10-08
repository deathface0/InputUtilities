#include "inpututil/Key.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace inpututil {

namespace {

struct NamedKey {
    std::string_view name;
    Key key;
};

// The first entry of each key is its canonical name (returned by Key::name()).
// Letters, digits, F1-F24 and Numpad0-9 are handled in code.
constexpr NamedKey kNamedKeys[] = {
    {"NumpadAdd", Key::NumpadAdd},
    {"NumpadPlus", Key::NumpadAdd},
    {"NumpadSubtract", Key::NumpadSubtract},
    {"NumpadMinus", Key::NumpadSubtract},
    {"NumpadMultiply", Key::NumpadMultiply},
    {"NumpadDivide", Key::NumpadDivide},
    {"NumpadDecimal", Key::NumpadDecimal},
    {"NumpadEnter", Key::NumpadEnter},

    {"Ctrl", Key::Ctrl},
    {"Control", Key::Ctrl},
    {"LCtrl", Key::LCtrl},
    {"RCtrl", Key::RCtrl},
    {"Shift", Key::Shift},
    {"LShift", Key::LShift},
    {"RShift", Key::RShift},
    {"Alt", Key::Alt},
    {"LAlt", Key::LAlt},
    {"RAlt", Key::RAlt},
    {"AltGr", Key::RAlt},
    {"LWin", Key::LWin},
    {"Win", Key::LWin},
    {"Windows", Key::LWin},
    {"Meta", Key::LWin},
    {"Super", Key::LWin},
    {"RWin", Key::RWin},
    {"Apps", Key::Apps},
    {"ContextMenu", Key::Apps},

    {"Enter", Key::Enter},
    {"Return", Key::Enter},
    {"Esc", Key::Esc},
    {"Escape", Key::Esc},
    {"Tab", Key::Tab},
    {"Space", Key::Space},
    {"Backspace", Key::Backspace},
    {"CapsLock", Key::CapsLock},
    {"NumLock", Key::NumLock},
    {"ScrollLock", Key::ScrollLock},
    {"PrintScreen", Key::PrintScreen},
    {"PrtSc", Key::PrintScreen},
    {"Pause", Key::Pause},
    {"Break", Key::Pause},

    {"Insert", Key::Insert},
    {"Ins", Key::Insert},
    {"Delete", Key::Delete},
    {"Del", Key::Delete},
    {"Home", Key::Home},
    {"End", Key::End},
    {"PageUp", Key::PageUp},
    {"PgUp", Key::PageUp},
    {"PageDown", Key::PageDown},
    {"PgDn", Key::PageDown},
    {"Left", Key::Left},
    {"Up", Key::Up},
    {"Right", Key::Right},
    {"Down", Key::Down},

    {"Plus", Key::OemPlus},
    {"+", Key::OemPlus},
    {"Minus", Key::OemMinus},
    {"-", Key::OemMinus},
    {"Comma", Key::OemComma},
    {",", Key::OemComma},
    {"Period", Key::OemPeriod},
    {".", Key::OemPeriod},

    {"VolumeMute", Key::VolumeMute},
    {"Mute", Key::VolumeMute},
    {"VolumeDown", Key::VolumeDown},
    {"VolumeUp", Key::VolumeUp},
    {"MediaNext", Key::MediaNext},
    {"MediaPrev", Key::MediaPrev},
    {"MediaStop", Key::MediaStop},
    {"MediaPlayPause", Key::MediaPlayPause},
    {"PlayPause", Key::MediaPlayPause},
};

char lower(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

bool iequals(std::string_view a, std::string_view b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
                                              [](char x, char y) { return lower(x) == lower(y); });
}

bool istartsWith(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && iequals(text.substr(0, prefix.size()), prefix);
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}

// Decimal number made only of digits, or nullopt.
std::optional<int> parseDecimal(std::string_view text) {
    if (text.empty() || text.size() > 3) return std::nullopt;
    int value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return std::nullopt;
        value = value * 10 + (c - '0');
    }
    return value;
}

// Hexadecimal number with optional 0x prefix, up to 0xFFFF.
std::optional<std::uint16_t> parseHex(std::string_view text) {
    if (istartsWith(text, "0x")) text.remove_prefix(2);
    if (text.empty() || text.size() > 4) return std::nullopt;
    unsigned value = 0;
    for (char c : text) {
        const char l = lower(c);
        unsigned digit;
        if (l >= '0' && l <= '9') digit = static_cast<unsigned>(l - '0');
        else if (l >= 'a' && l <= 'f') digit = static_cast<unsigned>(l - 'a' + 10);
        else return std::nullopt;
        value = value * 16 + digit;
    }
    return static_cast<std::uint16_t>(value);
}

std::string hexName(const char* prefix, unsigned value) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), value > 0xFF ? "%s0x%04X" : "%s0x%02X", prefix, value);
    return buffer;
}

} // namespace

std::optional<Key> Key::parse(std::string_view name) {
    name = trim(name);
    if (name.empty()) return std::nullopt;

    // Single letter or digit
    if (name.size() == 1) {
        const char c = name.front();
        if (std::isalpha(static_cast<unsigned char>(c)))
            return fromVk(static_cast<std::uint16_t>(std::toupper(static_cast<unsigned char>(c))));
        if (c >= '0' && c <= '9') return fromVk(static_cast<std::uint16_t>(c));
    }

    for (const NamedKey& named : kNamedKeys)
        if (iequals(name, named.name)) return named.key;

    // Raw codes: vk:0x41, sc:0x11, sc:0xE04B
    if (istartsWith(name, "vk:")) {
        const auto value = parseHex(name.substr(3));
        if (!value || *value == 0 || *value > 0xFF) return std::nullopt;
        return fromVk(*value);
    }
    if (istartsWith(name, "sc:")) {
        const auto value = parseHex(name.substr(3));
        if (!value || (*value & 0xFF) == 0) return std::nullopt;
        if (*value > 0xFF && (*value & 0xFF00) != 0xE000) return std::nullopt;
        return fromScanCode(*value);
    }

    // F1-F24, Numpad0-9 / Num0-9
    if (name.size() >= 2 && lower(name.front()) == 'f') {
        if (const auto n = parseDecimal(name.substr(1)); n && *n >= 1 && *n <= 24)
            return fromVk(static_cast<std::uint16_t>(0x70 + *n - 1));
        return std::nullopt;
    }
    for (std::string_view prefix : {std::string_view("Numpad"), std::string_view("Num")}) {
        if (istartsWith(name, prefix) && name.size() == prefix.size() + 1) {
            const char d = name.back();
            if (d >= '0' && d <= '9') return fromVk(static_cast<std::uint16_t>(0x60 + (d - '0')));
        }
    }
    return std::nullopt;
}

std::string Key::name() const {
    if (!valid()) return {};

    if (!isScanCode()) {
        const std::uint16_t code = vk();
        if ((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9')) return std::string(1, static_cast<char>(code));
        if (code >= 0x70 && code <= 0x87) return "F" + std::to_string(code - 0x70 + 1);
        if (code >= 0x60 && code <= 0x69) return "Numpad" + std::to_string(code - 0x60);
    }
    for (const NamedKey& named : kNamedKeys)
        if (named.key == *this) return std::string(named.name);

    if (isScanCode()) return hexName("sc:", scanCode() | (extended() ? 0xE000u : 0u));
    return hexName("vk:", vk());
}

std::optional<Chord> Chord::parse(std::string_view text) {
    Chord chord;
    std::size_t i = 0;
    const auto skipSpaces = [&] {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
    };

    while (true) {
        skipSpaces();
        if (i >= text.size()) return std::nullopt; // empty text or trailing '+'

        // A token starting with '+' is the Plus key itself ("Ctrl++").
        const std::size_t start = i;
        if (text[i] == '+') {
            ++i;
        } else {
            while (i < text.size() && text[i] != '+') ++i;
        }

        const auto key = Key::parse(text.substr(start, i - start));
        if (!key) return std::nullopt;
        if (std::find(chord.keys.begin(), chord.keys.end(), *key) != chord.keys.end()) return std::nullopt;
        chord.keys.push_back(*key);

        skipSpaces();
        if (i >= text.size()) return chord;
        if (text[i] != '+') return std::nullopt;
        ++i; // separator
    }
}

std::string Chord::toString() const {
    std::string text;
    for (const Key& key : keys) {
        if (!text.empty()) text += '+';
        text += key.name();
    }
    return text;
}

} // namespace inpututil
