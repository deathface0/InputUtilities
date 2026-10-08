#include "TestSupport.h"

#include <inpututil/Key.h>

using inpututil::KeyCombo;
using inpututil::Key;

namespace {

struct NamedConstant {
    const char* constant;
    Key key;
};

// Every Key constant, generated from the same list as the constants.
#define TEST_KEY_ENTRY(name, expr) {#name, Key::name},
const NamedConstant kAllNamedKeys[] = {INPUTUTIL_NAMED_KEYS(TEST_KEY_ENTRY)};
#undef TEST_KEY_ENTRY

} // namespace

TEST_CASE("Key::parse accepts names in any case with surrounding spaces") {
    CHECK(Key::parse("ctrl") == Key::Ctrl);
    CHECK(Key::parse("CONTROL") == Key::Ctrl);
    CHECK(Key::parse("  Ctrl ") == Key::Ctrl);
    CHECK(Key::parse("a") == Key::A);
    CHECK(Key::parse("Z") == Key::Z);
    CHECK(Key::parse("7") == Key::Digit7);
    CHECK(Key::parse("f5") == Key::F5);
    CHECK(Key::parse("F24") == Key::F24);
    CHECK(Key::parse("F") == Key::F);
    CHECK(Key::parse("num3") == Key::Numpad3);
    CHECK(Key::parse("Numpad0") == Key::Numpad0);
    CHECK(Key::parse("pgup") == Key::PageUp);
    CHECK(Key::parse("escape") == Key::Esc);
    CHECK(Key::parse("altgr") == Key::RAlt);
    CHECK(Key::parse("super") == Key::LWin);
    CHECK(Key::parse("+") == Key::OemPlus);
    CHECK(Key::parse("plus") == Key::OemPlus);
}

TEST_CASE("Key::parse accepts raw virtual keys and scan codes") {
    CHECK(Key::parse("vk:0x41") == Key::A);
    CHECK(Key::parse("VK:41") == Key::A);
    CHECK(Key::parse("sc:0x11") == Key::fromScanCode(0x11));
    CHECK(Key::parse("sc:0xE04B") == Key::fromScanCode(0x4B, true));
}

TEST_CASE("Key::parse rejects unknown names") {
    for (const char* text : {"", "   ", "foo", "F0", "F25", "vk:zz", "vk:0x10000", "vk:0x100", "vk:0", "sc:0x1234",
                             "ctrl+c", "Num10"}) {
        CAPTURE(text);
        CHECK_FALSE(Key::parse(text));
    }
}

TEST_CASE("Key::name gives the name of the constant or the raw form") {
    CHECK(Key::Ctrl.name() == "Ctrl");
    CHECK(Key::A.name() == "A");
    CHECK(Key::Digit7.name() == "Digit7");
    CHECK(Key::F5.name() == "F5");
    CHECK(Key::Numpad4.name() == "Numpad4");
    CHECK(Key::AltGr.name() == "RAlt");
    CHECK(Key::NumpadEnter.name() == "NumpadEnter");
    CHECK(Key::OemPlus.name() == "OemPlus");
    CHECK(Key::fromVk(0xE9).name() == "vk:0xE9");
    CHECK(Key::fromScanCode(0x76).name() == "sc:0x76");
    CHECK(Key::fromScanCode(0x4B, true).name() == "sc:0xE04B");
    CHECK(Key{}.name().empty());
}

TEST_CASE("every constant parses from its own name") {
    for (const auto& [constant, key] : kAllNamedKeys) {
        CAPTURE(constant);
        CHECK(Key::parse(constant) == key);
    }
}

TEST_CASE("every named key survives a name round trip") {
    for (const auto& [constant, key] : kAllNamedKeys) {
        CAPTURE(constant);
        CHECK(Key::parse(key.name()) == key);
    }
    CHECK(Key::parse(Key::fromScanCode(0x4B, true).name()) == Key::fromScanCode(0x4B, true));
}

TEST_CASE("KeyCombo::parse reads keys in press order") {
    const auto combo = KeyCombo::parse("Ctrl+Shift+Esc");
    REQUIRE(combo);
    REQUIRE(combo->keys.size() == 3);
    CHECK(combo->keys[0] == Key::Ctrl);
    CHECK(combo->keys[1] == Key::Shift);
    CHECK(combo->keys[2] == Key::Esc);

    CHECK(KeyCombo::parse(" alt + f4 ") == KeyCombo{{Key::Alt, Key::F4}});
    CHECK(KeyCombo::parse("Ctrl++") == KeyCombo{{Key::Ctrl, Key::OemPlus}});
    CHECK(KeyCombo::parse("+") == KeyCombo{{Key::OemPlus}});
    CHECK(KeyCombo::parse("F5") == KeyCombo{{Key::F5}});
}

TEST_CASE("KeyCombo::parse rejects malformed combos") {
    for (const char* text : {"", " ", "Ctrl+", "+Ctrl", "Ctrl+Ctrl", "Ctrl+foo", "Ctrl Shift"}) {
        CAPTURE(text);
        CHECK_FALSE(KeyCombo::parse(text));
    }
}

TEST_CASE("KeyCombo::toString joins names and round trips") {
    const KeyCombo combo{{Key::Ctrl, Key::Shift, Key::Esc}};
    CHECK(combo.toString() == "Ctrl+Shift+Esc");
    CHECK(KeyCombo::parse(combo.toString()) == combo);

    const KeyCombo withPlus{{Key::Ctrl, Key::OemPlus}};
    CHECK(withPlus.toString() == "Ctrl+OemPlus");
    CHECK(KeyCombo::parse(withPlus.toString()) == withPlus);
}
