#include "detail/Text.h"

#include "detail/InputBuilders.h"

namespace inpututil::detail {

namespace {

bool isHighSurrogate(wchar_t c) { return c >= 0xD800 && c <= 0xDBFF; }
bool isLowSurrogate(wchar_t c) { return c >= 0xDC00 && c <= 0xDFFF; }

// VkKeyScan modifier bits
constexpr unsigned kShift = 1;
constexpr unsigned kCtrl = 2;
constexpr unsigned kAlt = 4;

INPUT unicodeEvent(wchar_t unit, bool up) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = unit;
    input.ki.dwFlags = KEYEVENTF_UNICODE | (up ? KEYEVENTF_KEYUP : 0);
    return input;
}

// A surrogate pair goes in the same group, so the target receives both halves together.
InputGroup unicodeGroup(std::wstring_view units) {
    InputGroup group;
    for (wchar_t unit : units) {
        group.push_back(unicodeEvent(unit, false));
        group.push_back(unicodeEvent(unit, true));
    }
    return group;
}

std::optional<InputGroup> keyTapGroup(Key key, KeyMode mode, Backend& backend) {
    const auto events = makeComboEvents(std::span(&key, 1), mode, backend);
    if (!events) return std::nullopt;
    return events->tap();
}

// The real keys of the active layout for a BMP character, wrapped in the
// modifiers it needs. nullopt if the layout has no single key for it.
std::optional<InputGroup> keystrokeGroup(wchar_t ch, KeyMode mode, Backend& backend) {
    const std::int16_t scan = backend.vkKeyScan(ch);
    if (scan == -1) return std::nullopt;

    const auto vk = static_cast<std::uint16_t>(scan & 0xFF);
    unsigned modifiers = (static_cast<std::uint16_t>(scan) >> 8) & 0xFF;
    // No key, or modifiers beyond Shift/Ctrl/Alt (Kana, ...).
    if (vk == 0 || vk == 0xFF || (modifiers & ~(kShift | kCtrl | kAlt)) != 0) return std::nullopt;
    // A dead key types nothing by itself: it would combine with the next character.
    if (backend.isDeadKey(vk, modifiers)) return std::nullopt;

    // With Caps Lock on, letters need the opposite Shift state.
    if (backend.isKeyToggled(VK_CAPITAL) && IsCharAlphaW(ch)) modifiers ^= kShift;

    std::vector<Key> keys;
    if (modifiers & kShift) keys.push_back(Key::LShift);
    if ((modifiers & (kCtrl | kAlt)) == (kCtrl | kAlt)) {
        keys.push_back(Key::LCtrl); // AltGr
        keys.push_back(Key::RAlt);
    } else {
        if (modifiers & kCtrl) keys.push_back(Key::LCtrl);
        if (modifiers & kAlt) keys.push_back(Key::LAlt);
    }
    keys.push_back(Key::fromVk(vk));

    const auto events = makeComboEvents(keys, mode, backend);
    if (!events) return std::nullopt;
    return events->tap();
}

} // namespace

std::optional<std::wstring> utf8ToUtf16(std::string_view utf8) {
    if (utf8.empty()) return std::wstring{};
    const int size = static_cast<int>(utf8.size());
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), size, nullptr, 0);
    if (length <= 0) return std::nullopt;

    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), size, wide.data(), length);
    return wide;
}

Status buildTextGroups(std::wstring_view text, TextMode mode, bool fallbackToUnicode, KeyMode keyMode,
                       Backend& backend, std::vector<InputGroup>& groups) {
    groups.clear();
    for (std::size_t i = 0; i < text.size(); ++i) {
        const wchar_t ch = text[i];

        // Line breaks and tabs are keys: many applications ignore them as Unicode.
        if (ch == L'\r' || ch == L'\n' || ch == L'\t') {
            if (ch == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n') ++i;
            const auto group = keyTapGroup(ch == L'\t' ? Key::Tab : Key::Enter, keyMode, backend);
            if (!group) return Error::InvalidArgument;
            groups.push_back(*group);
            continue;
        }

        // Other control characters would become shortcuts in Keystrokes mode
        // (U+0001 is Ctrl+A, U+0016 Ctrl+V) and raw control codes as Unicode.
        if (ch < 0x20 || ch == 0x7F) return Error::InvalidArgument;

        if (isLowSurrogate(ch)) return Error::InvalidArgument;
        if (isHighSurrogate(ch)) {
            if (i + 1 >= text.size() || !isLowSurrogate(text[i + 1])) return Error::InvalidArgument;
            // Characters outside the BMP (emoji) have no key in any layout.
            if (mode == TextMode::Keystrokes && !fallbackToUnicode) return Error::UnmappableCharacter;
            groups.push_back(unicodeGroup(text.substr(i, 2)));
            ++i;
            continue;
        }

        if (mode == TextMode::Keystrokes) {
            if (auto group = keystrokeGroup(ch, keyMode, backend)) {
                groups.push_back(std::move(*group));
                continue;
            }
            if (!fallbackToUnicode) return Error::UnmappableCharacter;
        }
        groups.push_back(unicodeGroup(std::wstring_view(&ch, 1)));
    }
    return {};
}

} // namespace inpututil::detail
