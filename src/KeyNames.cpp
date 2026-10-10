#include "inpututil/Key.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <span>

namespace inpututil {

namespace {

struct NamedKey {
    std::string_view name;
    Key key;
};

// One entry per Key constant, generated from KeyList.h. The first match is
// the name returned by Key::name().
#define INPUTUTIL_KEY_ENTRY(name, expr) {#name, Key::name},
constexpr NamedKey kNamedKeys[] = {INPUTUTIL_NAMED_KEYS(INPUTUTIL_KEY_ENTRY)};
#undef INPUTUTIL_KEY_ENTRY

// Alternative spellings accepted by Key::parse (never returned by name()).
constexpr NamedKey kAliases[] = {
    {"Control", Key::Ctrl},
    {"Escape", Key::Esc},
    {"Return", Key::Enter},
    {"Windows", Key::LWin},
    {"Meta", Key::LWin},
    {"Super", Key::LWin},
    {"ContextMenu", Key::Apps},
    {"PrtSc", Key::PrintScreen},
    {"Break", Key::Pause},
    {"Ins", Key::Insert},
    {"Del", Key::Delete},
    {"PgUp", Key::PageUp},
    {"PgDn", Key::PageDown},
    {"NumpadPlus", Key::NumpadAdd},
    {"NumpadMinus", Key::NumpadSubtract},
    {"Plus", Key::OemPlus},
    {"+", Key::OemPlus},
    {"Minus", Key::OemMinus},
    {"-", Key::OemMinus},
    {"Comma", Key::OemComma},
    {",", Key::OemComma},
    {"Period", Key::OemPeriod},
    {".", Key::OemPeriod},
    {"Mute", Key::VolumeMute},
    {"PlayPause", Key::MediaPlayPause},
};

char lower(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

bool iequals(std::string_view a, std::string_view b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return lower(x) == lower(y); });
}

bool istartsWith(std::string_view text, std::string_view prefix) {
    return text.size() >= prefix.size() && iequals(text.substr(0, prefix.size()), prefix);
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}

std::optional<Key> findIn(std::span<const NamedKey> table, std::string_view name) {
    for (const NamedKey& named : table)
        if (iequals(name, named.name)) return named.key;
    return std::nullopt;
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

    // A single letter or digit: "a", "7"
    if (name.size() == 1) {
        const char c = name.front();
        if (std::isalpha(static_cast<unsigned char>(c)))
            return fromVk(static_cast<std::uint16_t>(std::toupper(static_cast<unsigned char>(c))));
        if (c >= '0' && c <= '9') return fromVk(static_cast<std::uint16_t>(c));
    }

    if (const auto key = findIn(kNamedKeys, name)) return key;
    if (const auto key = findIn(kAliases, name)) return key;

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

    // Short keypad names: Num0..Num9
    if (name.size() == 4 && istartsWith(name, "Num") && name.back() >= '0' && name.back() <= '9')
        return fromVk(static_cast<std::uint16_t>(0x60 + (name.back() - '0')));

    return std::nullopt;
}

std::string Key::name() const {
    if (!valid()) return {};

    for (const NamedKey& named : kNamedKeys)
        if (named.key == *this) return std::string(named.name);

    if (isScanCode()) return hexName("sc:", scanCode() | (extended() ? 0xE000u : 0u));
    return hexName("vk:", vk());
}

std::optional<KeyCombo> KeyCombo::parse(std::string_view text) {
    KeyCombo combo;
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
        if (std::find(combo.keys.begin(), combo.keys.end(), *key) != combo.keys.end()) return std::nullopt;
        combo.keys.push_back(*key);

        skipSpaces();
        if (i >= text.size()) return combo;
        if (text[i] != '+') return std::nullopt;
        ++i; // separator
    }
}

std::string KeyCombo::toString() const {
    std::string text;
    for (const Key& key : keys) {
        if (!text.empty()) text += '+';
        text += key.name();
    }
    return text;
}

} // namespace inpututil
