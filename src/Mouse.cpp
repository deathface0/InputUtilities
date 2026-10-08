#include "inpututil/Mouse.h"

#include <random>

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

bool Mouse::isHeld(MouseButton button) const {
    return session_ && session_->isHeld(detail::makeButtonInput(button, false));
}

} // namespace inpututil
