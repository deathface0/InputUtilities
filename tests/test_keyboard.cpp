#include "TestSupport.h"

#include "FakeBackend.h"

#include <inpututil/inpututil.h>

using inpututil::KeyCombo;
using inpututil::Config;
using inpututil::Error;
using inpututil::Hold;
using inpututil::Input;
using inpututil::Key;
using inpututil::KeyMode;
using namespace std::chrono_literals;

namespace {

std::shared_ptr<FakeBackend> makeFake() {
    auto fake = std::make_shared<FakeBackend>();
    fake->loadUsScanCodes();
    return fake;
}

struct Fixture {
    std::shared_ptr<FakeBackend> fake = makeFake();
    Input input{Config{.backend = fake}};
};

bool isUp(const INPUT& in) { return (in.ki.dwFlags & KEYEVENTF_KEYUP) != 0; }

} // namespace

TEST_CASE_FIXTURE(Fixture, "down and up send one event each in the configured mode") {
    REQUIRE(input.keyboard.down(Key::A));
    REQUIRE(input.keyboard.up(Key::A));
    auto sent = fake->allSent();
    REQUIRE(sent.size() == 2);
    CHECK(sent[0].ki.wVk == 'A');
    CHECK(sent[0].ki.wScan == 0x1E);
    CHECK_FALSE(isUp(sent[0]));
    CHECK(isUp(sent[1]));

    input.keyboard.setMode(KeyMode::ScanCode);
    CHECK(input.keyboard.mode() == KeyMode::ScanCode);
    REQUIRE(input.keyboard.down(Key::Left));
    sent = fake->allSent();
    CHECK(sent.back().ki.wVk == 0);
    CHECK(sent.back().ki.dwFlags == (KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY));
}

TEST_CASE_FIXTURE(Fixture, "invalid keys and combos send nothing") {
    CHECK(input.keyboard.down(Key{}) == Error::InvalidArgument);
    CHECK(input.keyboard.tap(Key{}) == Error::InvalidArgument);
    CHECK(input.keyboard.press("Ctrl+foo") == Error::InvalidArgument);
    const KeyCombo empty;
    const KeyCombo withInvalidKey{{Key::Ctrl, Key{}}};
    CHECK(input.keyboard.press(empty) == Error::InvalidArgument);
    CHECK(input.keyboard.press(withInvalidKey) == Error::InvalidArgument);
    CHECK(input.keyboard.hold("nope").status() == Error::InvalidArgument);
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "tap sends press and release in one batch, or two with a hold time") {
    REQUIRE(input.keyboard.tap(Key::A));
    REQUIRE(fake->batches.size() == 1);
    CHECK(fake->batches[0].size() == 2);

    const auto start = fake->now();
    REQUIRE(input.keyboard.tap(Key::A, 50ms));
    CHECK(fake->batches.size() == 3);
    CHECK(fake->now() - start == 50ms);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "press sends the whole combo atomically and releases in reverse") {
    REQUIRE(input.keyboard.press("Ctrl+Shift+Esc"));
    REQUIRE(fake->batches.size() == 1);
    const auto& batch = fake->batches[0];
    REQUIRE(batch.size() == 6);

    const WORD expected[] = {VK_CONTROL, VK_SHIFT, VK_ESCAPE, VK_ESCAPE, VK_SHIFT, VK_CONTROL};
    for (std::size_t i = 0; i < 6; ++i) {
        CAPTURE(i);
        CHECK(batch[i].ki.wVk == expected[i]);
        CHECK(isUp(batch[i]) == (i >= 3));
    }
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "press with a hold time keeps the combo down in between") {
    const auto start = fake->now();
    const KeyCombo copy{{Key::Ctrl, Key::C}};
    REQUIRE(input.keyboard.press(copy, 100ms));
    REQUIRE(fake->batches.size() == 2);
    CHECK(fake->batches[0].size() == 2);
    CHECK(fake->batches[1].size() == 2);
    CHECK(fake->batches[1][0].ki.wVk == 'C');
    CHECK(fake->now() - start == 100ms);
}

TEST_CASE_FIXTURE(Fixture, "a partially sent combo still releases every key") {
    fake->acceptLimit = 2;
    CHECK(input.keyboard.press("Ctrl+Shift+Esc") == Error::PartialSend);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "hold keeps a key down until the Hold goes away") {
    {
        auto shift = input.keyboard.hold(Key::Shift);
        CHECK(shift.active());
        CHECK(shift.status());
        CHECK(input.keyboard.isHeld(Key::Shift));
        CHECK(input.heldCount() == 1);
    }
    CHECK_FALSE(input.keyboard.isHeld(Key::Shift));
    CHECK(input.heldCount() == 0);
    CHECK(fake->batches.size() == 2);
}

TEST_CASE_FIXTURE(Fixture, "a Hold releases exactly once") {
    auto shift = input.keyboard.hold(Key::Shift);
    REQUIRE(shift.release());
    CHECK_FALSE(shift.active());
    REQUIRE(shift.release()); // no-op
    CHECK(fake->batches.size() == 2);

    auto ctrl = input.keyboard.hold(Key::Ctrl);
    Hold moved = std::move(ctrl);
    CHECK_FALSE(ctrl.active());
    CHECK(moved.active());
    moved = Hold{}; // releases Ctrl
    CHECK(input.heldCount() == 0);
    CHECK(fake->batches.size() == 4);
}

TEST_CASE_FIXTURE(Fixture, "holding a combo releases it in reverse order") {
    auto combo = input.keyboard.hold("Ctrl+Alt");
    CHECK(input.heldCount() == 2);
    REQUIRE(combo.release());

    const auto& releases = fake->batches.back();
    REQUIRE(releases.size() == 2);
    CHECK(releases[0].ki.wVk == VK_MENU);
    CHECK(releases[1].ki.wVk == VK_CONTROL);
}

TEST_CASE("a Hold can outlive its Input") {
    auto fake = makeFake();
    Hold shift;
    {
        Input input(Config{.backend = fake});
        shift = input.keyboard.hold(Key::Shift);
    } // the Input releases Shift itself
    const auto batches = fake->batches.size();
    CHECK(shift.release());
    CHECK(fake->batches.size() == batches);
}

TEST_CASE("releaseOnDestroy decides whether held keys are released") {
    auto fake = makeFake();
    {
        Input input(Config{.backend = fake});
        REQUIRE(input.keyboard.down(Key::A));
    }
    REQUIRE(fake->batches.size() == 2);
    CHECK(isUp(fake->batches[1][0]));

    fake->batches.clear();
    {
        Input input(Config{.releaseOnDestroy = false, .backend = fake});
        REQUIRE(input.keyboard.down(Key::A));
    }
    CHECK(fake->batches.size() == 1);
}

TEST_CASE("a moved Input keeps working and the moved-from one is empty") {
    auto fake = makeFake();
    Input source(Config{.keyMode = KeyMode::ScanCode, .backend = fake});
    REQUIRE(source.keyboard.down(Key::A));

    Input target = std::move(source);
    CHECK(target.heldCount() == 1);
    CHECK(target.keyboard.mode() == KeyMode::ScanCode);
    CHECK(source.keyboard.down(Key::B) == Error::InvalidArgument);
    CHECK(source.releaseAll() == Error::InvalidArgument);
    CHECK(source.heldCount() == 0);

    REQUIRE(target.keyboard.up(Key::A));
    CHECK(target.heldCount() == 0);
}

TEST_CASE("Input uses the Win32 backend by default") {
    Input input; // no input is sent: only construction and queries
    CHECK(input.heldCount() == 0);
    CHECK_FALSE(input.keyboard.isHeld(Key::A));
}
