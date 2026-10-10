#include "inpututil/Mouse.h"

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

// One button event; a release goes best effort so a button is never left down.
Status sendButton(detail::Session* session, MouseButton button, bool up) {
    if (!session) return Error::InvalidArgument;
    const INPUT input = detail::makeButtonInput(button, up);
    const auto sendMode = up ? detail::SendMode::BestEffort : detail::SendMode::StopOnFailure;
    return session->send(std::span(&input, 1), sendMode);
}

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
    // Where the cursor is just before the last move, to tell when Windows has applied it.
    const std::optional<Point> previous =
        path.size() > 1 ? detail::clampToScreen(path[path.size() - 2].point, screen) : backend.cursorPos();
    const auto absolute = [&screen](Point, Point next) { return detail::makeAbsoluteMove(next, screen); };
    if (const Status status = detail::playPath(*session_, from, path, absolute); !status) return status;
    if (previous) detail::settleCursor(backend, goal, *previous);

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

Status Mouse::down(MouseButton button) { return sendButton(session_, button, false); }

Status Mouse::up(MouseButton button) { return sendButton(session_, button, true); }

Hold Mouse::hold(MouseButton button) {
    if (const Status status = down(button); !status) return Hold(nullptr, status);
    return Hold(detail::makeReleaser(*session_, {detail::makeButtonInput(button, true)}), Status{});
}

Status Mouse::click(MouseButton button, const ClickOptions& options) {
    if (!session_) return Error::InvalidArgument;
    if (options.count < 1) return Error::InvalidArgument;

    const INPUT downEvent = detail::makeButtonInput(button, false);
    const INPUT upEvent = detail::makeButtonInput(button, true);
    const auto zero = std::chrono::milliseconds::zero();

    // Sends the events; if the system rejects them, releases the button before failing.
    const auto sendOrRelease = [&](std::span<const INPUT> events) {
        const Status status = session_->send(events);
        if (!status) session_->release(std::span(&upEvent, 1));
        return status;
    };

    // No timing at all: every click in one batch.
    if (options.hold <= zero && options.interval <= zero) {
        std::vector<INPUT> batch;
        for (int i = 0; i < options.count; ++i) {
            batch.push_back(downEvent);
            batch.push_back(upEvent);
        }
        return sendOrRelease(batch);
    }

    for (int i = 0; i < options.count; ++i) {
        if (i > 0) {
            if (const Status status = session_->wait(options.interval); !status) return status;
        }

        if (options.hold <= zero) {
            const INPUT clickEvents[] = {downEvent, upEvent};
            if (const Status status = sendOrRelease(clickEvents); !status) return status;
            continue;
        }

        if (const Status status = sendOrRelease(std::span(&downEvent, 1)); !status) return status;
        if (const Status status = session_->wait(options.hold); !status) return status; // aborted: released
        if (const Status status = session_->release(std::span(&upEvent, 1)); !status) return status;
    }
    return {};
}

Status Mouse::doubleClick(MouseButton button, std::chrono::milliseconds interval) {
    if (!session_) return Error::InvalidArgument;
    // not a double click
    if (interval >= session_->backend().doubleClickTime()) return Error::InvalidArgument;
    return click(button, {.count = 2, .interval = interval});
}

Status Mouse::drag(Point from, Point to, const Motion& motion, MouseButton button) {
    if (!session_) return Error::InvalidArgument;
    const Point start = detail::clampToScreen(from, session_->backend().virtualScreen());

    if (const Status status = moveAbsolute(start, start, Motion::instant()); !status) return status;
    if (const Status status = down(button); !status) return status;

    const Status moved = moveAbsolute(start, to, motion);
    // Always released, even if the movement failed (unless the abort key already did it).
    const INPUT upEvent = detail::makeButtonInput(button, true);
    const Status released = session_->release(std::span(&upEvent, 1));
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
        const auto deadline =
            start + std::chrono::duration_cast<Backend::Clock::duration>(length * i / steps);
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
