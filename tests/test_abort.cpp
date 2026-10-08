#include "TestSupport.h"

#include "FakeBackend.h"

#include <inpututil/inpututil.h>

using inpututil::Config;
using inpututil::Error;
using inpututil::Input;
using inpututil::Key;
using inpututil::Motion;
using inpututil::MouseButton;
using inpututil::Point;
using inpututil::Sequence;
using namespace std::chrono_literals;

namespace {

struct Fixture {
    std::shared_ptr<FakeBackend> fake = [] {
        auto backend = std::make_shared<FakeBackend>();
        backend->loadUsLayout();
        return backend;
    }();
    Input input{Config{.abortKey = Key::F12, .backend = fake}};

    void pressF12After(std::chrono::milliseconds delay) { fake->pressedFrom[VK_F12] = fake->now() + delay; }
};

} // namespace

TEST_CASE("without an abort key nothing is aborted") {
    auto fake = std::make_shared<FakeBackend>();
    fake->keysDown.insert(VK_F12);
    Input input(Config{.backend = fake});
    CHECK(input.mouse.moveTo({200, 0}, Motion::linear(100ms)));
    CHECK(fake->cursor == Point{200, 0});
}

TEST_CASE_FIXTURE(Fixture, "a long movement stops when the abort key goes down") {
    const auto start = fake->now();
    pressF12After(300ms);

    CHECK(input.mouse.moveTo({1000, 0}, Motion::linear(1000ms)) == Error::Aborted);
    const auto elapsed = fake->now() - start;
    CHECK(elapsed >= 300ms);
    CHECK(elapsed <= 310ms);
    CHECK(fake->batches.size() >= 55);
    CHECK(fake->batches.size() <= 61);
    CHECK_FALSE(fake->cursor == Point{1000, 0});
}

TEST_CASE_FIXTURE(Fixture, "aborting releases everything that is held") {
    REQUIRE(input.keyboard.down(Key::Shift));
    pressF12After(100ms);
    CHECK(input.keyboard.tap(Key::A, 1000ms) == Error::Aborted);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "typing checks the abort key before every character") {
    SUBCASE("already down: nothing is typed") {
        fake->keysDown.insert(VK_F12);
        CHECK(input.keyboard.type("abcdef") == Error::Aborted);
        CHECK(fake->batches.empty());
    }
    SUBCASE("pressed midway: typing stops there") {
        pressF12After(120ms);
        CHECK(input.keyboard.type("abcdef", {.delay = 50ms}) == Error::Aborted);
        CHECK(fake->batches.size() == 3); // a, b, c at 0, 50 and 100 ms
    }
}

TEST_CASE_FIXTURE(Fixture, "a sequence stops at a wait and leaves nothing held") {
    const auto start = fake->now();
    pressF12After(200ms);
    CHECK(input.play(Sequence{}.down(Key::Ctrl).wait(1s).up(Key::Ctrl)) == Error::Aborted);
    CHECK_FALSE(input.keyboard.isHeld(Key::Ctrl));
    CHECK(fake->now() - start >= 200ms);
    CHECK(fake->now() - start <= 210ms);

    fake->batches.clear();
    CHECK(input.play(Sequence{}.tap(Key::A)) == Error::Aborted); // still held: does not start
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "timed clicks and timed scrolling are aborted too") {
    pressF12After(150ms);
    CHECK(input.mouse.click(MouseButton::Left, {.count = 5, .interval = 100ms}) == Error::Aborted);
    CHECK(input.heldCount() == 0);

    fake->pressedFrom.clear();
    pressF12After(50ms);
    CHECK(input.mouse.scroll(10, 500ms) == Error::Aborted);
}

TEST_CASE_FIXTURE(Fixture, "instant actions ignore the abort key") {
    fake->keysDown.insert(VK_F12);
    CHECK(input.keyboard.tap(Key::A));
    CHECK(input.mouse.click());
    CHECK(input.mouse.moveTo({10, 10}));
    CHECK(fake->batches.size() == 3);
}

TEST_CASE_FIXTURE(Fixture, "the abort key can be changed or removed") {
    fake->keysDown.insert(VK_F12);

    input.setAbortKey(std::nullopt);
    CHECK(input.mouse.moveTo({100, 0}, Motion::linear(20ms)));

    input.setAbortKey(Key::Esc);
    CHECK(input.mouse.moveTo({0, 0}, Motion::linear(20ms)));
    fake->keysDown.insert(VK_ESCAPE);
    CHECK(input.mouse.moveTo({100, 0}, Motion::linear(20ms)) == Error::Aborted);

    // A physical (scan code) key is translated to its virtual key.
    fake->keysDown.clear();
    input.setAbortKey(Key::fromScanCode(0x01)); // Esc in the US table
    fake->keysDown.insert(VK_ESCAPE);
    CHECK(input.mouse.moveTo({300, 0}, Motion::linear(20ms)) == Error::Aborted);
}
