#include "TestSupport.h"

#include "FakeBackend.h"
#include "detail/InputBuilders.h"
#include "detail/Session.h"

#include <inpututil/Key.h>

using inpututil::Key;
using inpututil::KeyMode;
using inpututil::detail::isExtendedVk;
using inpututil::detail::makeKeyInput;

TEST_CASE("named keys carry the Win32 virtual key codes") {
    CHECK(Key::A.vk() == 'A');
    CHECK(Key::Digit7.vk() == '7');
    CHECK(Key::F24.vk() == VK_F24);
    CHECK(Key::Ctrl.vk() == VK_CONTROL);
    CHECK(Key::RAlt.vk() == VK_RMENU);
    CHECK(Key::Left.vk() == VK_LEFT);
    CHECK(Key::NumpadDivide.vk() == VK_DIVIDE);
    CHECK(Key::MediaPlayPause.vk() == VK_MEDIA_PLAY_PAUSE);
    CHECK(Key::AltGr == Key::RAlt);
    CHECK(Key::Win == Key::LWin);
}

TEST_CASE("keys created from scan codes keep their physical position") {
    CHECK_FALSE(Key{}.valid());

    const Key w = Key::fromScanCode(0x11);
    CHECK(w.valid());
    CHECK(w.isScanCode());
    CHECK(w.scanCode() == 0x11);
    CHECK(w.vk() == 0);
    CHECK_FALSE(w.extended());

    CHECK(Key::fromScanCode(0xE04B) == Key::fromScanCode(0x4B, true));
    CHECK(Key::NumpadEnter.scanCode() == 0x1C);
    CHECK(Key::NumpadEnter.extended());
}

TEST_CASE("extended virtual keys") {
    for (const Key key : {Key::Left, Key::Up, Key::Right, Key::Down, Key::Insert, Key::Delete, Key::Home,
                          Key::End, Key::PageUp, Key::PageDown, Key::RCtrl, Key::RAlt, Key::LWin, Key::NumLock,
                          Key::NumpadDivide, Key::PrintScreen, Key::VolumeUp}) {
        CAPTURE(key.vk());
        CHECK(isExtendedVk(key.vk()));
    }
    for (const Key key : {Key::A, Key::Numpad4, Key::LCtrl, Key::Ctrl, Key::Enter, Key::F1}) {
        CAPTURE(key.vk());
        CHECK_FALSE(isExtendedVk(key.vk()));
    }
}

TEST_CASE("building key events") {
    FakeBackend fake;
    fake.loadUsScanCodes();

    SUBCASE("letter in virtual-key mode carries vk and scan code") {
        const auto in = makeKeyInput(Key::A, KeyMode::VirtualKey, false, fake);
        REQUIRE(in);
        CHECK(in->type == INPUT_KEYBOARD);
        CHECK(in->ki.wVk == 'A');
        CHECK(in->ki.wScan == 0x1E);
        CHECK(in->ki.dwFlags == 0);
    }
    SUBCASE("arrow is extended even though Windows does not report the prefix") {
        const auto vkMode = makeKeyInput(Key::Left, KeyMode::VirtualKey, false, fake);
        REQUIRE(vkMode);
        CHECK(vkMode->ki.wVk == VK_LEFT);
        CHECK(vkMode->ki.wScan == 0x4B);
        CHECK(vkMode->ki.dwFlags == KEYEVENTF_EXTENDEDKEY);

        const auto scanMode = makeKeyInput(Key::Left, KeyMode::ScanCode, false, fake);
        REQUIRE(scanMode);
        CHECK(scanMode->ki.wVk == 0);
        CHECK(scanMode->ki.wScan == 0x4B);
        CHECK(scanMode->ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY));
    }
    SUBCASE("right Ctrl as scan code") {
        const auto in = makeKeyInput(Key::RCtrl, KeyMode::ScanCode, false, fake);
        REQUIRE(in);
        CHECK(in->ki.wScan == 0x1D);
        CHECK(in->ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY));
    }
    SUBCASE("Print Screen uses E0 37, not the Alt+SysRq code Windows reports") {
        const auto in = makeKeyInput(Key::PrintScreen, KeyMode::ScanCode, false, fake);
        REQUIRE(in);
        CHECK(in->ki.wScan == 0x37);
        CHECK(in->ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY));
    }
    SUBCASE("Pause cannot be a single scan code, so it is sent as a virtual key") {
        const auto in = makeKeyInput(Key::Pause, KeyMode::ScanCode, false, fake);
        REQUIRE(in);
        CHECK(in->ki.wVk == VK_PAUSE);
        CHECK(in->ki.wScan == 0);
        CHECK((in->ki.dwFlags & KEYEVENTF_SCANCODE) == 0);
    }
    SUBCASE("release adds KEYUP") {
        const auto in = makeKeyInput(Key::A, KeyMode::ScanCode, true, fake);
        REQUIRE(in);
        CHECK(in->ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP));
    }
    SUBCASE("scan-code key in virtual-key mode gets its vk from the layout") {
        const auto in = makeKeyInput(Key::fromScanCode(0x1E), KeyMode::VirtualKey, false, fake);
        REQUIRE(in);
        CHECK(in->ki.wVk == 'A');
        CHECK(in->ki.wScan == 0x1E);
    }
    SUBCASE("scan code without vk falls back to a scan-code event") {
        const auto in = makeKeyInput(Key::fromScanCode(0x76), KeyMode::VirtualKey, false, fake);
        REQUIRE(in);
        CHECK(in->ki.wVk == 0);
        CHECK(in->ki.wScan == 0x76);
        CHECK(in->ki.dwFlags == KEYEVENTF_SCANCODE);
    }
    SUBCASE("vk without scan code falls back to a virtual-key event") {
        const auto in = makeKeyInput(Key::F20, KeyMode::ScanCode, false, fake);
        REQUIRE(in);
        CHECK(in->ki.wVk == VK_F20);
        CHECK(in->ki.dwFlags == 0);
    }
    SUBCASE("invalid key") {
        CHECK_FALSE(makeKeyInput(Key{}, KeyMode::VirtualKey, false, fake));
    }
}

TEST_CASE("a key pressed in one mode and released in the other is no longer held") {
    auto fake = std::make_shared<FakeBackend>();
    fake->loadUsScanCodes();
    inpututil::detail::Session session(fake, 0x49555449);

    const INPUT down[] = {*makeKeyInput(Key::Left, KeyMode::VirtualKey, false, *fake)};
    const INPUT up[] = {*makeKeyInput(Key::Left, KeyMode::ScanCode, true, *fake)};
    REQUIRE(session.send(down));
    CHECK(session.heldCount() == 1);
    REQUIRE(session.send(up));
    CHECK(session.heldCount() == 0);
}
