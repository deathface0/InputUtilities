#include "TestSupport.h"

#include "FakeBackend.h"
#include "detail/InputBuilders.h"
#include "detail/Path.h"
#include "detail/Session.h"

#include <algorithm>
#include <cmath>

#include <inpututil/Motion.h>

using inpututil::Easing;
using inpututil::Error;
using inpututil::Motion;
using inpututil::Point;
using inpututil::detail::PathStep;
using inpututil::detail::planPath;
using inpututil::detail::playPath;
using namespace std::chrono_literals;

namespace {

constexpr Easing kAllEasings[] = {Easing::Linear, Easing::SmoothStep, Easing::EaseInOutCubic};

std::vector<PathStep> plan(Point from, Point to, const Motion& motion, unsigned seed = 1) {
    std::mt19937 rng(seed);
    return planPath(from, to, motion, rng);
}

// Distance from p to the line through a and b.
double distanceToLine(Point p, Point a, Point b) {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    return std::abs(dy * (p.x - a.x) - dx * (p.y - a.y)) / std::hypot(dx, dy);
}

} // namespace

TEST_CASE("easings start at 0, end at 1 and never go backwards") {
    for (const Easing easing : kAllEasings) {
        CAPTURE(static_cast<int>(easing));
        CHECK(inpututil::easingValue(easing, 0.0) == doctest::Approx(0.0));
        CHECK(inpututil::easingValue(easing, 1.0) == doctest::Approx(1.0));
        double previous = 0.0;
        for (int i = 1; i <= 100; ++i) {
            const double value = inpututil::easingValue(easing, i / 100.0);
            CHECK(value >= previous - 1e-12);
            previous = value;
        }
    }
    for (const Easing easing : {Easing::SmoothStep, Easing::EaseInOutCubic}) {
        for (const double t : {0.1, 0.25, 0.4}) {
            CHECK(inpututil::easingValue(easing, 1.0 - t) == doctest::Approx(1.0 - inpututil::easingValue(easing, t)));
        }
    }
}

TEST_CASE("Motion presets") {
    CHECK(Motion::instant().duration == 0ms);

    constexpr Motion linear = Motion::linear(100ms);
    CHECK(linear.duration == 100ms);
    CHECK(linear.easing == Easing::Linear);
    CHECK_FALSE(linear.curved);

    CHECK(Motion::smooth(50ms).easing == Easing::SmoothStep);

    constexpr Motion human = Motion::human(400ms);
    CHECK(human.easing == Easing::EaseInOutCubic);
    CHECK(human.curved);
    CHECK(human.jitterPx == 1);

    constexpr Motion custom{.duration = 300ms, .easing = Easing::Linear, .stepInterval = 2ms};
    CHECK(custom.stepInterval == 2ms);
}

TEST_CASE("instant motion and zero distance are a single step") {
    const auto instant = plan({0, 0}, {300, 200}, Motion::instant());
    REQUIRE(instant.size() == 1);
    CHECK(instant[0].point == Point{300, 200});
    CHECK(instant[0].at == 0ns);

    CHECK(plan({50, 50}, {50, 50}, Motion::smooth(200ms)).size() == 1);
}

TEST_CASE("a linear motion has one evenly timed step per interval along the line") {
    const auto path = plan({0, 0}, {200, 100}, Motion::linear(100ms));
    REQUIRE(path.size() == 20);
    for (std::size_t i = 0; i < path.size(); ++i) {
        CAPTURE(i);
        CHECK(path[i].at == std::chrono::milliseconds(5 * (i + 1)));
        CHECK(distanceToLine(path[i].point, {0, 0}, {200, 100}) <= 1.0);
        if (i > 0) CHECK(path[i].point.x >= path[i - 1].point.x);
    }
    CHECK(path.back().point == Point{200, 100});
    CHECK(path[9].point == Point{100, 50}); // halfway at half the time
}

TEST_CASE("a step interval longer than the duration gives a single timed step") {
    const auto path = plan({0, 0}, {10, 10}, {.duration = 3ms, .stepInterval = 10ms});
    REQUIRE(path.size() == 1);
    CHECK(path[0].point == Point{10, 10});
    CHECK(path[0].at == 3ms);
}

TEST_CASE("curved paths are reproducible, bend away from the line and stay near it") {
    const Point from{100, 100};
    const Point to{900, 500};
    const Motion motion{.duration = 400ms, .curved = true};

    const auto first = plan(from, to, motion, 7);
    const auto second = plan(from, to, motion, 7);
    REQUIRE(first.size() == second.size());
    for (std::size_t i = 0; i < first.size(); ++i) CHECK(first[i].point == second[i].point);
    CHECK(first.back().point == to);

    double maxDeviation = 0.0;
    const double distance = std::hypot(to.x - from.x, to.y - from.y);
    const double margin = 0.25 * distance + 1.0;
    for (const PathStep& step : first) {
        maxDeviation = std::max(maxDeviation, distanceToLine(step.point, from, to));
        CHECK(step.point.x >= from.x - margin);
        CHECK(step.point.x <= to.x + margin);
        CHECK(step.point.y >= from.y - margin);
        CHECK(step.point.y <= to.y + margin);
    }
    CHECK(maxDeviation > 5.0);
}

TEST_CASE("jitter moves intermediate points by at most jitterPx and never the last one") {
    const Motion straight = Motion::linear(200ms);
    Motion jittery = straight;
    jittery.jitterPx = 3;

    const auto base = plan({0, 0}, {400, 0}, straight, 3);
    const auto noisy = plan({0, 0}, {400, 0}, jittery, 3);
    REQUIRE(base.size() == noisy.size());
    bool moved = false;
    for (std::size_t i = 0; i < base.size(); ++i) {
        CHECK(std::abs(noisy[i].point.x - base[i].point.x) <= 3);
        CHECK(std::abs(noisy[i].point.y - base[i].point.y) <= 3);
        moved = moved || !(noisy[i].point == base[i].point);
    }
    CHECK(moved);
    CHECK(noisy.back().point == Point{400, 0});
}

TEST_CASE("playPath sends every point at its deadline") {
    auto fake = std::make_shared<FakeBackend>();
    inpututil::detail::Session session(fake, 0x49555449);
    const auto path = plan({0, 0}, {200, 100}, Motion::linear(100ms));
    const auto start = fake->now();

    const auto absolute = [&](Point, Point next) { return inpututil::detail::makeAbsoluteMove(next, fake->screen); };
    REQUIRE(playPath(session, {0, 0}, path, absolute));

    REQUIRE(fake->batchTimes.size() == path.size());
    for (std::size_t i = 0; i < path.size(); ++i) {
        CAPTURE(i);
        CHECK(fake->batchTimes[i] - start == path[i].at);
    }
    CHECK(fake->now() - start == 100ms);
    CHECK(fake->cursor == Point{200, 100});
}

TEST_CASE("late wake-ups do not accumulate") {
    auto fake = std::make_shared<FakeBackend>();
    fake->sleepOvershoot = 2ms;
    inpututil::detail::Session session(fake, 0x49555449);
    const auto path = plan({0, 0}, {200, 0}, Motion::linear(100ms)); // 20 steps
    const auto start = fake->now();

    const auto relative = [](Point previous, Point next) {
        return inpututil::detail::makeRelativeMove(next.x - previous.x, next.y - previous.y);
    };
    REQUIRE(playPath(session, {0, 0}, path, relative));

    CHECK(fake->now() - start == 102ms); // one overshoot, not twenty
    CHECK(fake->cursor == Point{200, 0});
}

TEST_CASE("playPath stops at the first rejected point") {
    auto fake = std::make_shared<FakeBackend>();
    fake->rejectAfter = 5;
    fake->errorCode = ERROR_ACCESS_DENIED;
    inpututil::detail::Session session(fake, 0x49555449);
    const auto path = plan({0, 0}, {200, 0}, Motion::linear(100ms));

    const auto absolute = [&](Point, Point next) { return inpututil::detail::makeAbsoluteMove(next, fake->screen); };
    const auto status = playPath(session, {0, 0}, path, absolute);
    CHECK(status == Error::SystemFailure);
    CHECK(status.win32Error() == ERROR_ACCESS_DENIED);
    CHECK(fake->batchTimes.size() == 6);
}
