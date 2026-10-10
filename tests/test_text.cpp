#include "TestSupport.h"

#include "FakeBackend.h"

#include <algorithm>
#include <cwctype>
#include <functional>

#include <inpututil/inpututil.h>

using inpututil::Config;
using inpututil::Error;
using inpututil::Input;
using inpututil::KeyMode;
using inpututil::TextMode;
using inpututil::TypeOptions;
using namespace std::chrono_literals;

namespace {

constexpr TypeOptions kKeystrokes{.mode = TextMode::Keystrokes};

struct Fixture {
    std::shared_ptr<FakeBackend> fake = FakeBackend::withUsLayout();
    Input input{Config{.backend = fake}};
};

bool isUnicode(const INPUT& in, wchar_t unit, bool up) {
    const DWORD flags = KEYEVENTF_UNICODE | (up ? KEYEVENTF_KEYUP : 0);
    return in.type == INPUT_KEYBOARD && in.ki.wVk == 0 && in.ki.wScan == unit && in.ki.dwFlags == flags;
}

// Virtual keys of a batch, as "vk↓"/"vk↑" pairs, for compact comparisons.
std::vector<std::pair<WORD, bool>> keysOf(const std::vector<INPUT>& batch) {
    std::vector<std::pair<WORD, bool>> keys;
    for (const INPUT& in : batch) keys.emplace_back(in.ki.wVk, isKeyUp(in));
    return keys;
}

using Keys = std::vector<std::pair<WORD, bool>>;

} // namespace

TEST_CASE_FIXTURE(Fixture, "Unicode mode sends one batch per character") {
    REQUIRE(input.keyboard.type("hi"));
    REQUIRE(fake->batches.size() == 2);
    REQUIRE(fake->batches[0].size() == 2);
    CHECK(isUnicode(fake->batches[0][0], L'h', false));
    CHECK(isUnicode(fake->batches[0][1], L'h', true));
    CHECK(isUnicode(fake->batches[1][0], L'i', false));
}

TEST_CASE_FIXTURE(Fixture, "characters outside the BMP go as a surrogate pair in one batch") {
    REQUIRE(input.keyboard.type(L"\U0001F600"));
    REQUIRE(fake->batches.size() == 1);
    const auto& batch = fake->batches[0];
    REQUIRE(batch.size() == 4);
    CHECK(isUnicode(batch[0], 0xD83D, false));
    CHECK(isUnicode(batch[1], 0xD83D, true));
    CHECK(isUnicode(batch[2], 0xDE00, false));
    CHECK(isUnicode(batch[3], 0xDE00, true));
}

TEST_CASE_FIXTURE(Fixture, "UTF-8, UTF-16 and u8 text produce the same events") {
    REQUIRE(input.keyboard.type("ñ€😀"));
    const auto utf8 = fake->allSent();
    fake->batches.clear();
    REQUIRE(input.keyboard.type(L"ñ€\U0001F600"));
    const auto utf16 = fake->allSent();
    fake->batches.clear();
    REQUIRE(input.keyboard.type(u8"ñ€😀"));
    const auto u8 = fake->allSent();

    REQUIRE(utf8.size() == 8);
    REQUIRE(utf16.size() == utf8.size());
    REQUIRE(u8.size() == utf8.size());
    for (std::size_t i = 0; i < utf8.size(); ++i) {
        CAPTURE(i);
        CHECK(utf8[i].ki.wScan == utf16[i].ki.wScan);
        CHECK(utf8[i].ki.wScan == u8[i].ki.wScan);
    }
}

TEST_CASE_FIXTURE(Fixture, "invalid text is rejected before anything is sent") {
    CHECK(input.keyboard.type("ok\xFF") == Error::InvalidArgument);
    const wchar_t loneHigh[] = {L'a', 0xD83D, L'b', 0};
    CHECK(input.keyboard.type(loneHigh) == Error::InvalidArgument);
    const wchar_t loneLow[] = {L'a', 0xDE00, 0};
    CHECK(input.keyboard.type(loneLow) == Error::InvalidArgument);
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "line breaks and tabs are typed as Enter and Tab") {
    REQUIRE(input.keyboard.type("a\r\nb\tc\nd\re"));
    REQUIRE(fake->batches.size() == 9);
    CHECK(isKeyEvent(fake->batches[1][0], VK_RETURN, false));
    CHECK(isKeyEvent(fake->batches[1][1], VK_RETURN, true));
    CHECK(isKeyEvent(fake->batches[3][0], VK_TAB, false));
    CHECK(isKeyEvent(fake->batches[5][0], VK_RETURN, false));
    CHECK(isKeyEvent(fake->batches[7][0], VK_RETURN, false));
    CHECK(isUnicode(fake->batches[8][0], L'e', false));
}

TEST_CASE_FIXTURE(Fixture, "keystrokes mode presses Shift where the layout needs it") {
    REQUIRE(input.keyboard.type("aA_", kKeystrokes));
    REQUIRE(fake->batches.size() == 3);
    CHECK(keysOf(fake->batches[0]) == Keys{{'A', false}, {'A', true}});
    CHECK(keysOf(fake->batches[1]) == Keys{{VK_LSHIFT, false}, {'A', false}, {'A', true}, {VK_LSHIFT, true}});
    CHECK(keysOf(fake->batches[2]) ==
          Keys{{VK_LSHIFT, false}, {VK_OEM_MINUS, false}, {VK_OEM_MINUS, true}, {VK_LSHIFT, true}});
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "keystrokes mode types PlayerOne_123 exactly (broken in v1)") {
    const std::wstring text = L"PlayerOne_123";
    REQUIRE(input.keyboard.type(text, kKeystrokes));
    REQUIRE(fake->batches.size() == text.size());

    for (std::size_t i = 0; i < text.size(); ++i) {
        const wchar_t ch = text[i];
        const bool needsShift = (ch >= L'A' && ch <= L'Z') || ch == L'_';
        const WORD vk = ch == L'_' ? WORD{VK_OEM_MINUS} : static_cast<WORD>(std::towupper(ch));
        CAPTURE(i);
        if (needsShift) {
            CHECK(keysOf(fake->batches[i]) ==
                  Keys{{VK_LSHIFT, false}, {vk, false}, {vk, true}, {VK_LSHIFT, true}});
        } else {
            CHECK(keysOf(fake->batches[i]) == Keys{{vk, false}, {vk, true}});
        }
    }
}

TEST_CASE("keystrokes mode uses AltGr and layout keys on a Spanish keyboard") {
    auto fake = std::make_shared<FakeBackend>();
    fake->loadEsLayout();
    Input input(Config{.backend = fake});

    REQUIRE(input.keyboard.type(L"@€ñ", kKeystrokes));
    REQUIRE(fake->batches.size() == 3);
    CHECK(keysOf(fake->batches[0]) == Keys{{VK_LCONTROL, false},
                                           {VK_RMENU, false},
                                           {'2', false},
                                           {'2', true},
                                           {VK_RMENU, true},
                                           {VK_LCONTROL, true}});
    CHECK(keysOf(fake->batches[1]) == Keys{{VK_LCONTROL, false},
                                           {VK_RMENU, false},
                                           {'E', false},
                                           {'E', true},
                                           {VK_RMENU, true},
                                           {VK_LCONTROL, true}});
    CHECK(keysOf(fake->batches[2]) == Keys{{VK_OEM_3, false}, {VK_OEM_3, true}});
    CHECK(fake->batches[2][0].ki.wScan == 0x27);
    CHECK(input.heldCount() == 0);
}

TEST_CASE("dead keys are typed as Unicode on a Spanish keyboard") {
    auto fake = std::make_shared<FakeBackend>();
    fake->loadEsLayout();
    Input input(Config{.backend = fake});

    // ^ and ~ are dead keys (they would turn "^a" into "â"); [ shares the key of ^ but is a normal character.
    REQUIRE(input.keyboard.type(L"^[~", kKeystrokes));
    REQUIRE(fake->batches.size() == 3);
    CHECK(isUnicode(fake->batches[0][0], L'^', false));
    CHECK(keysOf(fake->batches[1]) == Keys{{VK_LCONTROL, false},
                                           {VK_RMENU, false},
                                           {VK_OEM_1, false},
                                           {VK_OEM_1, true},
                                           {VK_RMENU, true},
                                           {VK_LCONTROL, true}});
    CHECK(isUnicode(fake->batches[2][0], L'~', false));

    fake->batches.clear();
    const TypeOptions strict{.mode = TextMode::Keystrokes, .fallbackToUnicode = false};
    CHECK(input.keyboard.type(L"a´", strict) == Error::UnmappableCharacter);
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "control characters are rejected before anything is sent") {
    // In Keystrokes mode they would become shortcuts: U+0001 is Ctrl+A, U+0016 Ctrl+V, U+001B Esc.
    for (const TextMode mode : {TextMode::Unicode, TextMode::Keystrokes}) {
        CAPTURE(static_cast<int>(mode));
        for (const wchar_t control : {L'\x01', L'\x16', L'\x1B', L'\x7F', L'\0'}) {
            CAPTURE(static_cast<int>(control));
            const wchar_t text[] = {L'a', control, L'b'};
            CHECK(input.keyboard.type(std::wstring_view(text, 3), {.mode = mode}) == Error::InvalidArgument);
        }
    }
    CHECK(input.keyboard.type("ok\x16") == Error::InvalidArgument);
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "Caps Lock inverts Shift for letters only") {
    fake->toggled.insert(VK_CAPITAL);
    REQUIRE(input.keyboard.type("Aa!", kKeystrokes));
    REQUIRE(fake->batches.size() == 3);
    CHECK(keysOf(fake->batches[0]) == Keys{{'A', false}, {'A', true}});
    CHECK(keysOf(fake->batches[1]) == Keys{{VK_LSHIFT, false}, {'A', false}, {'A', true}, {VK_LSHIFT, true}});
    CHECK(keysOf(fake->batches[2]) == Keys{{VK_LSHIFT, false}, {'1', false}, {'1', true}, {VK_LSHIFT, true}});
}

TEST_CASE_FIXTURE(Fixture, "characters without a key fall back to Unicode unless forbidden") {
    REQUIRE(input.keyboard.type(L"ñ😀", kKeystrokes));
    REQUIRE(fake->batches.size() == 2);
    CHECK(isUnicode(fake->batches[0][0], L'ñ', false));
    CHECK(isUnicode(fake->batches[1][0], 0xD83D, false));

    fake->batches.clear();
    const TypeOptions strict{.mode = TextMode::Keystrokes, .fallbackToUnicode = false};
    CHECK(input.keyboard.type(L"abñ", strict) == Error::UnmappableCharacter);
    CHECK(input.keyboard.type(L"ab\U0001F600", strict) == Error::UnmappableCharacter);
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "keystrokes follow the keyboard key mode") {
    input.keyboard.setMode(KeyMode::ScanCode);
    REQUIRE(input.keyboard.type("A", kKeystrokes));
    const auto& batch = fake->batches.at(0);
    REQUIRE(batch.size() == 4);
    for (const INPUT& in : batch) {
        CHECK(in.ki.wVk == 0);
        CHECK((in.ki.dwFlags & KEYEVENTF_SCANCODE) != 0);
    }
    CHECK(batch[0].ki.wScan == 0x2A); // left Shift
    CHECK(batch[1].ki.wScan == 0x1E); // A
}

TEST_CASE_FIXTURE(Fixture, "delay pauses between characters but not after the last one") {
    REQUIRE(input.keyboard.type("abc", {.delay = 20ms}));
    REQUIRE(fake->sleeps.size() == 2);
    CHECK(fake->sleeps[0] == 20ms);
    CHECK(fake->sleeps[1] == 20ms);
}

TEST_CASE("jitter is reproducible with a seed and never negative") {
    const auto run = [](TypeOptions options) {
        auto fake = std::make_shared<FakeBackend>();
        Input input(Config{.backend = fake});
        REQUIRE(input.keyboard.type("abcdefghij", options));
        return fake->sleeps;
    };

    const auto first = run({.delay = 30ms, .jitter = 10ms, .seed = 42});
    const auto second = run({.delay = 30ms, .jitter = 10ms, .seed = 42});
    REQUIRE(first.size() == 9);
    CHECK(first == second);
    for (const auto pause : first) {
        CHECK(pause >= 20ms);
        CHECK(pause <= 40ms);
    }
    // not constant
    CHECK(std::adjacent_find(first.begin(), first.end(), std::not_equal_to<>()) != first.end());

    for (const auto pause : run({.delay = 5ms, .jitter = 20ms, .seed = 7})) {
        CHECK(pause > 0ms);
        CHECK(pause <= 25ms);
    }
}

TEST_CASE_FIXTURE(Fixture, "a partially sent character releases its modifiers") {
    fake->acceptLimit = 2; // Shift down and A down get through, the releases do not
    CHECK(input.keyboard.type("A", kKeystrokes) == Error::PartialSend);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "a rejected character sends no releases") {
    fake->acceptLimit = 0;
    CHECK(input.keyboard.type("A", kKeystrokes) == Error::SystemFailure);
    CHECK(fake->batches.size() == 1); // only the character itself: nothing went down
}

TEST_CASE_FIXTURE(Fixture, "empty text and a moved-from Input") {
    CHECK(input.keyboard.type(""));
    CHECK(fake->batches.empty());

    Input other = std::move(input);
    CHECK(input.keyboard.type("x") == Error::InvalidArgument);
    CHECK(other.keyboard.type("x"));
}
