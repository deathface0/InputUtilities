#include "inpututil/Mouse.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

#include "detail/Coords.h"
#include "detail/InputBuilders.h"
#include "detail/Path.h"
#include "detail/Session.h"

namespace inpututil {

namespace {

// Random numbers are only needed for curved or jittered paths.
std::mt19937 makeRng(const Motion& motion) {
    if (motion.seed != 0) return std::mt19937(motion.seed);
    if (motion.curved || motion.jitterPx > 0) return std::mt19937(std::random_device{}());
    return std::mt19937();
}

Status cursorUnavailable(Backend& backend) { return {Error::SystemFailure, backend.lastError()}; }

} // namespace

std::optional<Point> Mouse::position() const {
    if (!session_) return std::nullopt;
    return session_->backend().cursorPos();
}

Status Mouse::moveTo(Point target, const Motion& motion) {
    if (!session_) return Error::InvalidArgument;
    if (motion.duration <= std::chrono::milliseconds::zero()) return moveAbsolute(target, target, motion);

    const auto from = session_->backend().cursorPos();
    if (!from) return cursorUnavailable(session_->backend());
    return moveAbsolute(*from, target, motion);
}

Status Mouse::moveBy(int dx, int dy, const Motion& motion) {
    if (!session_) return Error::InvalidArgument;
    const auto from = session_->backend().cursorPos();
    if (!from) return cursorUnavailable(session_->backend());
    return moveAbsolute(*from, {from->x + dx, from->y + dy}, motion);
}

Status Mouse::moveAbsolute(Point from, Point target, const Motion& motion) {
    Backend& backend = session_->backend();
    const Rect screen = backend.virtualScreen();
    const Point goal = detail::clampToScreen(target, screen);

    auto rng = makeRng(motion);
    const auto path = detail::planPath(from, goal, motion, rng);
    const auto absolute = [&screen](Point, Point next) { return detail::makeAbsoluteMove(next, screen); };
    if (const Status status = detail::playPath(*session_, from, path, absolute); !status) return status;

    if (verifyCursor_) {
        const auto reached = backend.cursorPos();
        if (!reached) return cursorUnavailable(backend);
        if (*reached != goal) return Error::TargetNotReached;
    }
    return {};
}

Status Mouse::moveRaw(int dx, int dy, const Motion& motion) {
    if (!session_) return Error::InvalidArgument;
    if (dx == 0 && dy == 0) return {};

    // Planned as a path from (0,0) to (dx,dy); each event carries the step between two points.
    auto rng = makeRng(motion);
    const auto path = detail::planPath({0, 0}, {dx, dy}, motion, rng);
    const auto relative = [](Point previous, Point next) {
        return detail::makeRelativeMove(next.x - previous.x, next.y - previous.y);
    };
    return detail::playPath(*session_, {0, 0}, path, relative);
}

Status Mouse::down(MouseButton button) {
    if (!session_) return Error::InvalidArgument;
    const INPUT input = detail::makeButtonInput(button, false);
    return session_->send(std::span(&input, 1));
}

Status Mouse::up(MouseButton button) {
    if (!session_) return Error::InvalidArgument;
    const INPUT input = detail::makeButtonInput(button, true);
    return session_->send(std::span(&input, 1), detail::SendMode::BestEffort);
}

Hold Mouse::hold(MouseButton button) {
    if (const Status status = down(button); !status) return Hold(nullptr, status);

    auto release = [session = session_->weak_from_this(), up = detail::makeButtonInput(button, true)]() -> Status {
        if (const auto alive = session.lock()) return alive->send(std::span(&up, 1), detail::SendMode::BestEffort);
        return {}; // the Input is gone and already released everything
    };
    return Hold(std::move(release), Status{});
}

Status Mouse::click(MouseButton button, const ClickOptions& options) {
    if (!session_) return Error::InvalidArgument;
    if (options.count < 1) return Error::InvalidArgument;

    const INPUT down = detail::makeButtonInput(button, false);
    const INPUT up = detail::makeButtonInput(button, true);
    const auto zero = std::chrono::milliseconds::zero();

    // No timing at all: every click in one batch.
    if (options.hold <= zero && options.interval <= zero) {
        std::vector<INPUT> batch;
        for (int i = 0; i < options.count; ++i) {
            batch.push_back(down);
            batch.push_back(up);
        }
        const Status status = session_->send(batch);
        if (!status) session_->send(std::span(&up, 1), detail::SendMode::BestEffort);
        return status;
    }

    for (int i = 0; i < options.count; ++i) {
        if (i > 0) session_->wait(options.interval);

        if (options.hold <= zero) {
            const INPUT clickEvents[] = {down, up};
            if (const Status status = session_->send(clickEvents); !status) {
                session_->send(std::span(&up, 1), detail::SendMode::BestEffort);
                return status;
            }
            continue;
        }

        if (const Status status = session_->send(std::span(&down, 1)); !status) {
            session_->send(std::span(&up, 1), detail::SendMode::BestEffort);
            return status;
        }
        session_->wait(options.hold);
        if (const Status status = session_->send(std::span(&up, 1), detail::SendMode::BestEffort); !status)
            return status;
    }
    return {};
}

Status Mouse::doubleClick(MouseButton button, std::chrono::milliseconds interval) {
    if (!session_) return Error::InvalidArgument;
    if (interval >= session_->backend().doubleClickTime()) return Error::InvalidArgument; // not a double click
    return click(button, {.count = 2, .interval = interval});
}

Status Mouse::drag(Point from, Point to, const Motion& motion, MouseButton button) {
    if (!session_) return Error::InvalidArgument;
    const Point start = detail::clampToScreen(from, session_->backend().virtualScreen());

    if (const Status status = moveAbsolute(start, start, Motion::instant()); !status) return status;
    if (const Status status = down(button); !status) return status;

    const Status moved = moveAbsolute(start, to, motion);
    const Status released = up(button); // always, even if the movement failed
    return moved ? released : moved;
}

Status Mouse::scroll(double notches, std::chrono::milliseconds duration) {
    return scrollAxis(notches, duration, false);
}

Status Mouse::scrollHorizontal(double notches, std::chrono::milliseconds duration) {
    return scrollAxis(notches, duration, true);
}

Status Mouse::scrollAxis(double notches, std::chrono::milliseconds duration, bool horizontal) {
    if (!session_) return Error::InvalidArgument;
    const auto delta = detail::wheelDelta(notches);
    if (!delta) return Error::InvalidArgument;
    const int total = *delta;
    if (total == 0) return {};

    if (duration <= std::chrono::milliseconds::zero()) {
        const INPUT wheel = detail::makeWheelInput(total, horizontal);
        return session_->send(std::span(&wheel, 1));
    }

    // One notch per step (the last one carries the remainder), each at its own deadline.
    const int sign = total > 0 ? 1 : -1;
    const int magnitude = total * sign;
    const int steps = (magnitude + WHEEL_DELTA - 1) / WHEEL_DELTA;
    const auto start = session_->backend().now();
    const std::chrono::nanoseconds length = duration;

    for (int i = 1; i <= steps; ++i) {
        const int amount = i < steps ? WHEEL_DELTA : magnitude - WHEEL_DELTA * (steps - 1);
        const auto deadline = start + std::chrono::duration_cast<Backend::Clock::duration>(length * i / steps);
        if (const Status status = session_->waitUntil(deadline); !status) return status;

        const INPUT wheel = detail::makeWheelInput(amount * sign, horizontal);
        if (const Status status = session_->send(std::span(&wheel, 1)); !status) return status;
    }
    return {};
}

bool Mouse::isHeld(MouseButton button) const {
    return session_ && session_->isHeld(detail::makeButtonInput(button, false));
}

} // namespace inpututil
