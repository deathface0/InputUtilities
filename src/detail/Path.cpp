#include "detail/Path.h"

#include <algorithm>
#include <cmath>

#include "detail/Session.h"

namespace inpututil::detail {

namespace {

struct Vec {
    double x;
    double y;
};

Vec operator+(Vec a, Vec b) { return {a.x + b.x, a.y + b.y}; }
Vec operator*(Vec a, double k) { return {a.x * k, a.y * k}; }

Vec bezier(Vec p0, Vec c1, Vec c2, Vec p3, double t) {
    const double u = 1.0 - t;
    return p0 * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + p3 * (t * t * t);
}

Point round(Vec v) { return {static_cast<int>(std::lround(v.x)), static_cast<int>(std::lround(v.y))}; }

} // namespace

std::vector<PathStep> planPath(Point from, Point to, const Motion& motion, std::mt19937& rng) {
    using std::chrono::milliseconds;
    using std::chrono::nanoseconds;

    if (motion.duration <= milliseconds::zero() || from == to) return {{to, nanoseconds::zero()}};

    const auto interval = std::max(motion.stepInterval, milliseconds(1));
    const auto steps = std::max<long long>(
        1, std::llround(static_cast<double>(motion.duration.count()) / static_cast<double>(interval.count())));

    const Vec start{static_cast<double>(from.x), static_cast<double>(from.y)};
    const Vec end{static_cast<double>(to.x), static_cast<double>(to.y)};
    const Vec delta{end.x - start.x, end.y - start.y};

    // Control points at 30% and 70% of the way, pushed sideways for a curved path.
    Vec c1 = start + delta * 0.3;
    Vec c2 = start + delta * 0.7;
    if (motion.curved) {
        const double distance = std::hypot(delta.x, delta.y);
        const Vec normal{-delta.y / distance, delta.x / distance};
        std::uniform_real_distribution<double> offset(-0.25, 0.25);
        c1 = c1 + normal * (offset(rng) * distance);
        c2 = c2 + normal * (offset(rng) * distance);
    }

    std::uniform_int_distribution<int> jitter(-motion.jitterPx, motion.jitterPx);
    const nanoseconds duration = motion.duration;

    std::vector<PathStep> path;
    path.reserve(static_cast<std::size_t>(steps));
    for (long long i = 1; i <= steps; ++i) {
        const double e = easingValue(motion.easing, static_cast<double>(i) / static_cast<double>(steps));
        Point point = round(motion.curved ? bezier(start, c1, c2, end, e) : start + delta * e);
        if (i < steps && motion.jitterPx > 0) {
            point.x += jitter(rng);
            point.y += jitter(rng);
        }
        if (i == steps) point = to;
        path.push_back({point, duration * i / steps});
    }
    return path;
}

Status playPath(Session& session, Point from, const std::vector<PathStep>& path, const MoveEvent& makeEvent) {
    const auto start = session.backend().now();
    Point previous = from;
    for (const PathStep& step : path) {
        const auto deadline = start + std::chrono::duration_cast<Backend::Clock::duration>(step.at);
        if (const Status status = session.waitUntil(deadline); !status) return status;

        const INPUT event = makeEvent(previous, step.point);
        if (const Status status = session.send(std::span(&event, 1)); !status) return status;
        previous = step.point;
    }
    return {};
}

} // namespace inpututil::detail
