#pragma once

namespace inpututil {

/// A position in screen pixels (virtual desktop coordinates, may be negative
/// on multi-monitor setups).
struct Point {
    int x = 0;
    int y = 0;

    friend constexpr bool operator==(Point, Point) noexcept = default;
};

/// An axis-aligned rectangle in screen pixels.
struct Rect {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;

    constexpr bool contains(Point p) const noexcept {
        return p.x >= left && p.y >= top && p.x < left + width && p.y < top + height;
    }

    friend constexpr bool operator==(const Rect&, const Rect&) noexcept = default;
};

} // namespace inpututil
