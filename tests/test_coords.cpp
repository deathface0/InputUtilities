#include "TestSupport.h"

#include "FakeBackend.h"
#include "detail/Coords.h"
#include "detail/InputBuilders.h"
#include "detail/Session.h"

#include <inpututil/Input.h>

using inpututil::Point;
using inpututil::Rect;
using inpututil::detail::clampToScreen;
using inpututil::detail::makeAbsoluteMove;
using inpututil::detail::makeRelativeMove;
using inpututil::detail::toAbsolute;

namespace {

// The two ways Windows may turn a normalized coordinate into a pixel.
int pixelModelA(int normalized, int origin, int extent) {
    return origin + static_cast<int>(static_cast<long long>(normalized) * extent / 65536);
}
int pixelModelB(int normalized, int origin, int extent) {
    return origin + static_cast<int>(static_cast<long long>(normalized) * (extent - 1) / 65535);
}

} // namespace

TEST_CASE("every pixel maps back to itself under both rounding models") {
    for (const int extent : {800, 1080, 1366, 1920, 2560, 3840, 5760}) {
        CAPTURE(extent);
        int mismatches = 0;
        for (int px = 0; px < extent; ++px) {
            const int n = toAbsolute(px, 0, extent);
            if (n < 0 || n > 65535 || pixelModelA(n, 0, extent) != px || pixelModelB(n, 0, extent) != px)
                ++mismatches;
        }
        CHECK(mismatches == 0);
    }
}

TEST_CASE("the v1 formulas land one pixel short where toAbsolute does not") {
    const int v1Smooth = (1000 * 65535) / 1920;  // v1 smooth SetCursorPos
    const int v1Instant = (1000 * 65536) / 1920; // v1 instant SetCursorPos
    CHECK(pixelModelA(v1Smooth, 0, 1920) == 999);
    CHECK(pixelModelB(v1Instant, 0, 1920) == 999);

    const int n = toAbsolute(1000, 0, 1920);
    CHECK(pixelModelA(n, 0, 1920) == 1000);
    CHECK(pixelModelB(n, 0, 1920) == 1000);
}

TEST_CASE("virtual desktops with a negative origin") {
    const Rect screen{-1920, 0, 3840, 1080}; // a monitor to the left of the primary one
    for (const int px : {-1920, -1000, -1, 0, 1, 1000, 1919}) {
        CAPTURE(px);
        const int n = toAbsolute(px, screen.left, screen.width);
        CHECK(pixelModelA(n, screen.left, screen.width) == px);
        CHECK(pixelModelB(n, screen.left, screen.width) == px);
    }
}

TEST_CASE("coordinates outside the screen are clamped to its edges") {
    CHECK(toAbsolute(-50, 0, 1920) == toAbsolute(0, 0, 1920));
    CHECK(toAbsolute(5000, 0, 1920) == toAbsolute(1919, 0, 1920));
    CHECK(toAbsolute(1919, 0, 1920) == 65535);
    CHECK(toAbsolute(0, 0, 1920) >= 0);
    CHECK(toAbsolute(10, 0, 1) == 0);
    CHECK(toAbsolute(10, 0, 0) == 0);

    const Rect screen{-1920, 0, 3840, 1080};
    CHECK(clampToScreen({-3000, -5}, screen) == Point{-1920, 0});
    CHECK(clampToScreen({4000, 2000}, screen) == Point{1919, 1079});
    CHECK(clampToScreen({10, 20}, screen) == Point{10, 20});
}

TEST_CASE("move events carry the right flags") {
    const INPUT absolute = makeAbsoluteMove({1000, 500}, {0, 0, 1920, 1080});
    CHECK(absolute.type == INPUT_MOUSE);
    CHECK(absolute.mi.dwFlags == (MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK));
    CHECK(absolute.mi.dx == toAbsolute(1000, 0, 1920));
    CHECK(absolute.mi.dy == toAbsolute(500, 0, 1080));

    const INPUT relative = makeRelativeMove(-15, 30);
    CHECK(relative.type == INPUT_MOUSE);
    CHECK(relative.mi.dwFlags == MOUSEEVENTF_MOVE);
    CHECK(relative.mi.dx == -15);
    CHECK(relative.mi.dy == 30);
}

TEST_CASE("sent moves put the simulated cursor exactly on target") {
    auto fake = std::make_shared<FakeBackend>();
    fake->screen = {-1920, 0, 3840, 1080};
    inpututil::detail::Session session(fake, inpututil::kDefaultExtraInfoTag);

    for (const auto model : {FakeBackend::AbsoluteModel::A, FakeBackend::AbsoluteModel::B}) {
        fake->absoluteModel = model;
        for (const Point target : {Point{1000, 500}, Point{-1920, 0}, Point{1919, 1079}, Point{-1, 1}}) {
            CAPTURE(target.x);
            CAPTURE(target.y);
            const INPUT move[] = {makeAbsoluteMove(target, fake->screen)};
            REQUIRE(session.send(move));
            CHECK(fake->cursor == target);
        }
    }

    fake->cursor = {100, 100};
    const INPUT relative[] = {makeRelativeMove(25, -40)};
    REQUIRE(session.send(relative));
    CHECK(fake->cursor == Point{125, 60});
    CHECK(session.heldCount() == 0); // moves are not held inputs
}
