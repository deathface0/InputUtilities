#include "TestSupport.h"

#include "FakeBackend.h"

#include <inpututil/inpututil.h>

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

constexpr DWORD kAbsoluteMove = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;

} // namespace

TEST_CASE_FIXTURE(Fixture, "position reads the cursor") {
    fake->cursor = {123, 456};
    CHECK(input.mouse.position() == Point{123, 456});
    fake->cursorAvailable = false;
    CHECK_FALSE(input.mouse.position());
}

TEST_CASE_FIXTURE(Fixture, "an instant moveTo sends one absolute move to the exact pixel") {
    fake->screen = {-1920, 0, 3840, 1080};
    for (const Point target : {Point{1000, 500}, Point{-1500, 20}, Point{-1, 1079}}) {
        CAPTURE(target.x);
        fake->batches.clear();
        REQUIRE(input.mouse.moveTo(target));
        REQUIRE(fake->batches.size() == 1);
        CHECK(fake->batches[0][0].mi.dwFlags == kAbsoluteMove);
        CHECK(fake->cursor == target);
    }
}

TEST_CASE_FIXTURE(Fixture, "a timed moveTo follows the path for its whole duration") {
    fake->cursor = {0, 0};
    const auto start = fake->now();
    REQUIRE(input.mouse.moveTo({200, 100}, Motion::linear(100ms)));
    CHECK(fake->batches.size() == 20);
    CHECK(fake->cursor == Point{200, 100});
    CHECK(fake->now() - start == 100ms);
}

TEST_CASE_FIXTURE(Fixture, "targets outside the screen are clamped to its edge") {
    REQUIRE(input.mouse.moveTo({5000, -300}));
    CHECK(fake->cursor == Point{1919, 0});
}

TEST_CASE_FIXTURE(Fixture, "moveBy moves relative to the current position") {
    fake->cursor = {100, 100};
    REQUIRE(input.mouse.moveBy(50, -20));
    CHECK(fake->cursor == Point{150, 80});
    REQUIRE(input.mouse.moveBy(-30, 10, Motion::smooth(50ms)));
    CHECK(fake->cursor == Point{120, 90});
}

TEST_CASE_FIXTURE(Fixture, "moves that need the current position fail without it") {
    fake->cursorAvailable = false;
    fake->errorCode = ERROR_ACCESS_DENIED;

    const auto relative = input.mouse.moveBy(10, 10);
    CHECK(relative == Error::SystemFailure);
    CHECK(relative.win32Error() == ERROR_ACCESS_DENIED);
    CHECK(input.mouse.moveTo({50, 50}, Motion::smooth(50ms)) == Error::SystemFailure);
    CHECK(fake->batches.empty());

    CHECK(input.mouse.moveTo({50, 50})); // instant: the position is not needed
    CHECK(fake->batches.size() == 1);
}

TEST_CASE("verifyCursor reports a target that was not reached") {
    auto fake = std::make_shared<FakeBackend>();
    Input input(Config{.verifyCursor = true, .backend = fake});

    CHECK(input.mouse.moveTo({300, 300}));
    CHECK(input.mouse.moveTo({300, 300})); // already there: still success
    CHECK(input.mouse.moveBy(10, 0, Motion::linear(20ms)));

    fake->moveCursor = false;
    CHECK(input.mouse.moveTo({800, 600}) == Error::TargetNotReached);
}

TEST_CASE_FIXTURE(Fixture, "moveRaw sends relative steps that add up to the requested distance") {
    fake->cursor = {500, 500};
    REQUIRE(input.mouse.moveRaw(30, -10, Motion::linear(50ms)));
    const auto sent = fake->allSent();
    REQUIRE(sent.size() == 10);

    long sumX = 0;
    long sumY = 0;
    for (const INPUT& in : sent) {
        CHECK(in.mi.dwFlags == MOUSEEVENTF_MOVE);
        sumX += in.mi.dx;
        sumY += in.mi.dy;
    }
    CHECK(sumX == 30);
    CHECK(sumY == -10);
    CHECK(fake->cursor == Point{530, 490});

    fake->batches.clear();
    REQUIRE(input.mouse.moveRaw(0, 0));
    CHECK(fake->batches.empty());
}

TEST_CASE_FIXTURE(Fixture, "buttons send the right flags") {
    struct Expected {
        MouseButton button;
        DWORD down;
        DWORD up;
        DWORD data;
    };
    const Expected cases[] = {
        {MouseButton::Left, MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, 0},
        {MouseButton::Right, MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, 0},
        {MouseButton::Middle, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0},
        {MouseButton::X1, MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON1},
        {MouseButton::X2, MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON2},
    };
    for (const Expected& expected : cases) {
        CAPTURE(static_cast<int>(expected.button));
        fake->batches.clear();
        REQUIRE(input.mouse.down(expected.button));
        CHECK(input.mouse.isHeld(expected.button));
        REQUIRE(input.mouse.up(expected.button));
        CHECK_FALSE(input.mouse.isHeld(expected.button));

        const auto sent = fake->allSent();
        REQUIRE(sent.size() == 2);
        CHECK(sent[0].mi.dwFlags == expected.down);
        CHECK(sent[0].mi.mouseData == expected.data);
        CHECK(sent[1].mi.dwFlags == expected.up);
        CHECK(sent[1].mi.mouseData == expected.data);
    }
    CHECK(input.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "hold keeps a button down until the Hold goes away") {
    {
        auto right = input.mouse.hold(MouseButton::Right);
        CHECK(right.active());
        CHECK(input.mouse.isHeld(MouseButton::Right));
        CHECK(input.heldCount() == 1);
    }
    CHECK_FALSE(input.mouse.isHeld(MouseButton::Right));
    CHECK(fake->allSent().back().mi.dwFlags == MOUSEEVENTF_RIGHTUP);
}

TEST_CASE("a button left down is released when the Input is destroyed") {
    auto fake = std::make_shared<FakeBackend>();
    {
        Input input(Config{.backend = fake});
        REQUIRE(input.mouse.down(MouseButton::Left));
    }
    CHECK(fake->allSent().back().mi.dwFlags == MOUSEEVENTF_LEFTUP);
}

TEST_CASE("the same seed gives the same human movement") {
    const auto run = [] {
        auto fake = std::make_shared<FakeBackend>();
        Input input(Config{.backend = fake});
        Motion motion = Motion::human(200ms);
        motion.seed = 99;
        REQUIRE(input.mouse.moveTo({900, 700}, motion));
        return fake->allSent();
    };
    const auto first = run();
    const auto second = run();
    REQUIRE(first.size() == second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        CHECK(first[i].mi.dx == second[i].mi.dx);
        CHECK(first[i].mi.dy == second[i].mi.dy);
    }
}

TEST_CASE("the mouse of a moved-from Input is empty") {
    auto fake = std::make_shared<FakeBackend>();
    Input source(Config{.backend = fake});
    Input target = std::move(source);

    CHECK(source.mouse.moveTo({10, 10}) == Error::InvalidArgument);
    CHECK(source.mouse.down() == Error::InvalidArgument);
    CHECK_FALSE(source.mouse.position());
    CHECK(target.mouse.moveTo({10, 10}));
}
