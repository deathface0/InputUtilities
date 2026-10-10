#include "TestSupport.h"

#include "FakeBackend.h"

#include <inpututil/inpututil.h>

#include <limits>

using inpututil::Config;
using inpututil::Error;
using inpututil::Input;
using inpututil::Key;
using inpututil::KeyCombo;
using inpututil::Motion;
using inpututil::Point;
using inpututil::Sequence;
using inpututil::TextMode;
using namespace std::chrono_literals;

namespace {

struct Fixture {
    std::shared_ptr<FakeBackend> fake = FakeBackend::withUsLayout();
    Input input{Config{.backend = fake}};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "a sequence is a reusable value") {
    Sequence sequence;
    CHECK(sequence.empty());
    sequence.tap(Key::A).click().wait(10ms).type("x");
    CHECK(sequence.size() == 4);

    const Sequence copy = sequence;
    REQUIRE(input.play(copy));
    const auto first = fake->allSent();
    fake->batches.clear();
    REQUIRE(input.play(copy));
    const auto second = fake->allSent();

    REQUIRE(first.size() == second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].type == second[i].type);
        CHECK(first[i].ki.wVk == second[i].ki.wVk);
        CHECK(first[i].ki.wScan == second[i].ki.wScan);
    }
}

TEST_CASE_FIXTURE(Fixture, "consecutive instant steps go in one batch") {
    REQUIRE(input.play(Sequence{}.down(Key::Ctrl).tap(Key::C).up(Key::Ctrl).click()));
    REQUIRE(fake->batches.size() == 1);
    const auto& batch = fake->batches[0];
    REQUIRE(batch.size() == 6);
    CHECK(isKeyEvent(batch[0], VK_CONTROL, false));
    CHECK(isKeyEvent(batch[1], 'C', false));
    CHECK(isKeyEvent(batch[2], 'C', true));
    CHECK(isKeyEvent(batch[3], VK_CONTROL, true));
    CHECK(batch[4].mi.dwFlags == MOUSEEVENTF_LEFTDOWN);
    CHECK(batch[5].mi.dwFlags == MOUSEEVENTF_LEFTUP);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "a wait splits the batch") {
    const auto start = fake->now();
    REQUIRE(input.play(Sequence{}.tap(Key::A).wait(50ms).tap(Key::B)));
    REQUIRE(fake->batches.size() == 2);
    CHECK(fake->batchTimes[0] - start == 0ms);
    CHECK(fake->batchTimes[1] - start == 50ms);
}

TEST_CASE_FIXTURE(Fixture, "a bad step anywhere fails the sequence before anything is sent") {
    CHECK(input.play(Sequence{}.tap(Key::A).wait(10ms).press("Ctrl+foo")) == Error::InvalidArgument);
    CHECK(input.play(Sequence{}.tap(Key::A).type("ok\xFF")) == Error::InvalidArgument);
    CHECK(input.play(Sequence{}.tap(Key::A).type(
              L"ñ", {.mode = TextMode::Keystrokes, .fallbackToUnicode = false})) ==
          Error::UnmappableCharacter);
    CHECK(input.play(Sequence{}.tap(Key::A).click(inpututil::MouseButton::Left, {.count = 0})) ==
          Error::InvalidArgument);
    CHECK(input.play(Sequence{}.down(Key{})) == Error::InvalidArgument);
    CHECK(input.play(Sequence{}.up(Key{})) == Error::InvalidArgument);
    CHECK(input.play(Sequence{}.tap(Key::A).press(KeyCombo{})) == Error::InvalidArgument);
    CHECK(input.play(Sequence{}.tap(Key::A).press(KeyCombo{{Key::A, Key{}}})) == Error::InvalidArgument);
    CHECK(input.play(Sequence{}.tap(Key::A).scroll(std::numeric_limits<double>::infinity())) ==
          Error::InvalidArgument);
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "typing inside a sequence sends one batch per character") {
    REQUIRE(input.play(Sequence{}.type("ab")));
    CHECK(fake->batches.size() == 2);
}

TEST_CASE_FIXTURE(Fixture, "mouse steps") {
    SUBCASE("moveBy sees the position left by the previous moveTo") {
        REQUIRE(input.play(Sequence{}.moveTo({100, 100}).moveBy(10, 0)));
        CHECK(fake->cursor == Point{110, 100});
    }
    SUBCASE("moveTo with a motion follows a path") {
        REQUIRE(input.play(Sequence{}.moveTo({200, 0}, Motion::linear(50ms))));
        CHECK(fake->batches.size() == 10);
        CHECK(fake->cursor == Point{200, 0});
    }
    SUBCASE("instant scrolling joins the batch") {
        REQUIRE(input.play(Sequence{}.tap(Key::A).scroll(2).moveRaw(5, 5)));
        REQUIRE(fake->batches.size() == 1);
        const auto& batch = fake->batches[0];
        REQUIRE(batch.size() == 4);
        CHECK(batch[2].mi.dwFlags == MOUSEEVENTF_WHEEL);
        CHECK(static_cast<std::int32_t>(batch[2].mi.mouseData) == 240);
        CHECK(batch[3].mi.dwFlags == MOUSEEVENTF_MOVE);
    }
}

TEST_CASE_FIXTURE(Fixture, "a failure midway releases what the sequence pressed") {
    fake->acceptLimit = 2;
    CHECK(input.play(Sequence{}.down(Key::Shift).wait(10ms).press("Ctrl+C")) == Error::PartialSend);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "a failure never releases the user's own holds") {
    auto shift = input.keyboard.hold(Key::Shift);
    fake->acceptLimit = 1;
    CHECK(input.play(Sequence{}.down(Key::Ctrl).wait(10ms).tap(Key::A)) == Error::PartialSend);
    CHECK(input.keyboard.isHeld(Key::Shift));
    CHECK_FALSE(input.keyboard.isHeld(Key::Ctrl));
    CHECK_FALSE(input.keyboard.isHeld(Key::A));
}

TEST_CASE_FIXTURE(Fixture, "sendRaw sends the events as given and tracks them") {
    INPUT events[2]{};
    events[0].type = INPUT_KEYBOARD;
    events[0].ki.wVk = 'Q';
    events[0].ki.wScan = 0x10;
    events[1].type = INPUT_MOUSE;
    events[1].mi.dwFlags = MOUSEEVENTF_MOVE;
    events[1].mi.dx = 3;

    REQUIRE(input.sendRaw(events));
    REQUIRE(fake->batches.size() == 1);
    CHECK(fake->batches[0][0].ki.wVk == 'Q');
    CHECK(fake->batches[0][0].ki.dwExtraInfo == inpututil::kDefaultExtraInfoTag);
    CHECK(fake->batches[0][1].mi.dx == 3);
    CHECK(input.heldCount() == 1); // Q is down

    REQUIRE(input.releaseAll());
    CHECK(input.heldCount() == 0);
    CHECK((fake->allSent().back().ki.dwFlags & KEYEVENTF_KEYUP) != 0);

    CHECK(input.sendRaw({}));
}

TEST_CASE("play and sendRaw on a moved-from Input") {
    auto fake = std::make_shared<FakeBackend>();
    Input source(Config{.backend = fake});
    Input target = std::move(source);
    CHECK(source.play(Sequence{}.tap(Key::A)) == Error::InvalidArgument);
    CHECK(source.sendRaw({}) == Error::InvalidArgument);
    CHECK(target.play(Sequence{}));
}
