#pragma once

#include <windows.h>

#include <chrono>
#include <functional>
#include <random>
#include <vector>

#include "inpututil/Backend.h"
#include "inpututil/Motion.h"
#include "inpututil/Point.h"
#include "inpututil/Status.h"

namespace inpututil::detail {

class Session;

struct PathStep {
    Point point;
    std::chrono::nanoseconds at; ///< time since the start of the movement
};

/// Points of a movement from `from` to `to` and when to reach each one. The
/// last point is always exactly `to`. An instant motion (or from == to) is a
/// single step at time zero.
std::vector<PathStep> planPath(Point from, Point to, const Motion& motion, std::mt19937& rng);

/// Builds the event that moves from one point of the path to the next.
using MoveEvent = std::function<INPUT(Point previous, Point next)>;

/// Sends the path on time: each step waits for its own deadline (start + at),
/// so late wake-ups never accumulate. Stops at the first error.
Status playPath(Session& session, Point from, const std::vector<PathStep>& path, const MoveEvent& makeEvent);

/// Windows applies an injected move asynchronously: right after SendInput the
/// cursor can still read as the old position (measured: 1 read in 10, for up
/// to 0.3 ms). Waits until it shows `goal` or at least leaves `previous` (a
/// DPI-virtualized position may not match `goal` exactly), at most 25 ms.
void settleCursor(Backend& backend, Point goal, Point previous);

} // namespace inpututil::detail
