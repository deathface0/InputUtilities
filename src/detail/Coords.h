#pragma once

#include "inpututil/Point.h"

namespace inpututil::detail {

/// Normalized absolute coordinate (0..65535) that Windows turns into exactly
/// pixel `px` of an axis starting at `origin` and `extent` pixels long. `px`
/// is clamped to the axis.
///
/// Windows does not document its rounding; the value works with both models
/// seen in practice: pixel = n * extent / 65536 and n * (extent - 1) / 65535
/// (both rounded down).
int toAbsolute(int px, int origin, int extent);

/// The point clamped to the rectangle.
Point clampToScreen(Point p, const Rect& screen);

} // namespace inpututil::detail
