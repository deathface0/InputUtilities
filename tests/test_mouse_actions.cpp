#include "TestSupport.h"

#include "FakeBackend.h"

#include <inpututil/inpututil.h>

using inpututil::ClickOptions;
using inpututil::Config;
using inpututil::Error;
using inpututil::Input;
using inpututil::Motion;
using inpututil::MouseButton;
using inpututil::Point;
using namespace std::chrono_literals;

namespace {

struct Fixture {
    std::shared_ptr<FakeBackend> fake = std::make_shared<FakeBackend>();
    Input input{Config{.backend = fake}};
};

int wheelDelta(const INPUT& in) { return static_cast<int>(static_cast<std::int32_t>(in.mi.mouseData)); }

} // namespace

TEST_CASE_FIXTURE(Fixture, "click sends press and release in one batch") {
    REQUIRE(input.mouse.click());
    REQUIRE(fake->batches.size() == 1);
    REQUIRE(fake->batches[0].size() == 2);
    CHECK(fake->batches[0][0].mi.dwFlags == MOUSEEVENTF_LEFTDOWN);
    CHECK(fake->batches[0][1].mi.dwFlags == MOUSEEVENTF_LEFTUP);

    fake->batches.clear();
    REQUIRE(input.mouse.click(MouseButton::Right, {.count = 3}));
    REQUIRE(fake->batches.size() == 1);
    const auto& batch = fake->batches[0];
    REQUIRE(batch.size() == 6);
    for (std::size_t i = 0; i < batch.size(); ++i)
        CHECK(batch[i].mi.dwFlags == (i % 2 == 0 ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP));
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "click with hold and interval follows the timing") {
    const auto start = fake->now();
    REQUIRE(input.mouse.click(MouseButton::Left, {.count = 2, .hold = 20ms, .interval = 30ms}));
    REQUIRE(fake->batchTimes.size() == 4);
    CHECK(fake->batchTimes[0] - start == 0ms);
    CHECK(fake->batchTimes[1] - start == 20ms);
    CHECK(fake->batchTimes[2] - start == 50ms);
    CHECK(fake->batchTimes[3] - start == 70ms);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "click needs at least one click") {
    CHECK(input.mouse.click(MouseButton::Left, {.count = 0}) == Error::InvalidArgument);
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "a partially sent multi-click still releases the button") {
    fake->acceptLimit = 1;
    CHECK(input.mouse.click(MouseButton::Left, {.count = 2}) == Error::PartialSend);
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "double click") {
    SUBCASE("instant: one batch of four events") {
        REQUIRE(input.mouse.doubleClick());
        REQUIRE(fake->batches.size() == 1);
        CHECK(fake->batches[0].size() == 4);
    }
    SUBCASE("with an interval inside the double-click time") {
        fake->doubleClick = 500ms;
        const auto start = fake->now();
        REQUIRE(input.mouse.doubleClick(MouseButton::Left, 100ms));
        REQUIRE(fake->batches.size() == 2);
        CHECK(fake->batchTimes[1] - start == 100ms);
    }
    SUBCASE("an interval Windows would not see as a double click is rejected") {
        fake->doubleClick = 500ms;
        CHECK(input.mouse.doubleClick(MouseButton::Left, 600ms) == Error::InvalidArgument);
        CHECK(input.mouse.doubleClick(MouseButton::Left, 500ms) == Error::InvalidArgument);
        CHECK(fake->batches.empty());
    }
}

TEST_CASE_FIXTURE(Fixture, "drag moves, presses, moves and releases") {
    for (const MouseButton button : {MouseButton::Left, MouseButton::Right}) {
        CAPTURE(static_cast<int>(button));
        fake->batches.clear();
        REQUIRE(input.mouse.drag({100, 100}, {400, 300}, Motion::linear(50ms), button));

        const auto sent = fake->allSent();
        REQUIRE(sent.size() == 13); // move to start, press, 10 moves, release
        const DWORD down = button == MouseButton::Left ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
        const DWORD up = button == MouseButton::Left ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;
        CHECK((sent[0].mi.dwFlags & MOUSEEVENTF_ABSOLUTE) != 0);
        CHECK(sent[1].mi.dwFlags == down);
        for (std::size_t i = 2; i < 12; ++i) CHECK((sent[i].mi.dwFlags & MOUSEEVENTF_MOVE) != 0);
        CHECK(sent[12].mi.dwFlags == up);
        CHECK(fake->cursor == Point{400, 300});
        CHECK(input.heldCount() == 0);
    }
}

TEST_CASE_FIXTURE(Fixture, "a drag that fails midway keeps the button tracked for a later release") {
    fake->rejectAfter = 5; // start move, press and three moves get through
    fake->errorCode = ERROR_ACCESS_DENIED;
    CHECK(input.mouse.drag({0, 0}, {500, 0}, Motion::linear(50ms)) == Error::SystemFailure);
    CHECK(input.mouse.isHeld(MouseButton::Left)); // the release was rejected too

    fake->rejectAfter.reset();
    REQUIRE(input.releaseAll());
    CHECK(input.heldCount() == 0);
    CHECK(fake->allSent().back().mi.dwFlags == MOUSEEVENTF_LEFTUP);
}

TEST_CASE_FIXTURE(Fixture, "instant scrolling sends one wheel event with the whole delta") {
    REQUIRE(input.mouse.scroll(3));
    REQUIRE(input.mouse.scroll(-1.5));
    REQUIRE(input.mouse.scroll(0.001)); // rounds to nothing
    REQUIRE(input.mouse.scrollHorizontal(2));

    const auto sent = fake->allSent();
    REQUIRE(sent.size() == 3);
    CHECK(sent[0].mi.dwFlags == MOUSEEVENTF_WHEEL);
    CHECK(wheelDelta(sent[0]) == 360);
    CHECK(wheelDelta(sent[1]) == -180);
    CHECK(sent[2].mi.dwFlags == MOUSEEVENTF_HWHEEL);
    CHECK(wheelDelta(sent[2]) == 240);
}

TEST_CASE_FIXTURE(Fixture, "scrolling over a duration spreads the notches evenly") {
    for (const double notches : {2.5, -2.5}) {
        CAPTURE(notches);
        fake->batches.clear();
        fake->batchTimes.clear();
        const auto start = fake->now();
        REQUIRE(input.mouse.scroll(notches, 90ms));

        const auto sent = fake->allSent();
        REQUIRE(sent.size() == 3);
        const int sign = notches > 0 ? 1 : -1;
        CHECK(wheelDelta(sent[0]) == 120 * sign);
        CHECK(wheelDelta(sent[1]) == 120 * sign);
        CHECK(wheelDelta(sent[2]) == 60 * sign);
        CHECK(fake->batchTimes[0] - start == 30ms);
        CHECK(fake->batchTimes[1] - start == 60ms);
        CHECK(fake->batchTimes[2] - start == 90ms);
    }
}
