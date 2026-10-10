#include "detail/Coords.h"

#include <algorithm>
#include <cstdint>

namespace inpututil::detail {

namespace {

constexpr std::int64_t kMaxNormalized = 65535;

std::int64_t ceilDiv(std::int64_t a, std::int64_t b) { return (a + b - 1) / b; }

} // namespace

int toAbsolute(int px, int origin, int extent) {
    if (extent <= 1) return 0;
    const std::int64_t i = std::clamp<std::int64_t>(std::int64_t{px} - origin, 0, extent - 1);
    const std::int64_t w = extent;

    // Normalized values that land on pixel i under each model:
    //   A: pixel = n * w / 65536          B: pixel = n * (w - 1) / 65535
    const std::int64_t loA = ceilDiv(i * 65536, w);
    const std::int64_t hiA = std::min(ceilDiv((i + 1) * 65536, w) - 1, kMaxNormalized);
    const std::int64_t loB = ceilDiv(i * kMaxNormalized, w - 1);
    const std::int64_t hiB = std::min(ceilDiv((i + 1) * kMaxNormalized, w - 1) - 1, kMaxNormalized);

    const std::int64_t lo = std::max(loA, loB);
    const std::int64_t hi = std::min(hiA, hiB);
    const std::int64_t value = lo <= hi ? (lo + hi) / 2 : (loA + hiA) / 2; // the ranges always overlap
    return static_cast<int>(std::clamp<std::int64_t>(value, 0, kMaxNormalized));
}

Point clampToScreen(Point p, const Rect& screen) {
    if (screen.width <= 0 || screen.height <= 0) return {screen.left, screen.top};
    return {std::clamp(p.x, screen.left, screen.left + screen.width - 1),
            std::clamp(p.y, screen.top, screen.top + screen.height - 1)};
}

} // namespace inpututil::detail
