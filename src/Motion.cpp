#include "inpututil/Motion.h"

#include <algorithm>

namespace inpututil {

double easingValue(Easing easing, double t) noexcept {
    t = std::clamp(t, 0.0, 1.0);
    switch (easing) {
    case Easing::Linear:
        return t;
    case Easing::SmoothStep:
        return t * t * (3.0 - 2.0 * t);
    case Easing::EaseInOutCubic: {
        if (t < 0.5) return 4.0 * t * t * t;
        const double u = -2.0 * t + 2.0;
        return 1.0 - u * u * u / 2.0;
    }
    }
    return t;
}

} // namespace inpututil
