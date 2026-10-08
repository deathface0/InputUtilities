#include "detail/InputBuilders.h"

#include "detail/Coords.h"

#include <algorithm>
#include <cmath>

namespace inpututil::detail {

bool isExtendedVk(std::uint16_t vk) {
    switch (vk) {
    case VK_RCONTROL:
    case VK_RMENU:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_DIVIDE:
    case VK_SNAPSHOT:
    case VK_CANCEL: // Ctrl+Break
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
        return true;
    default:
        // Browser, volume, media and launch keys
        return vk >= VK_BROWSER_BACK && vk <= VK_LAUNCH_APP2;
    }
}

std::optional<ResolvedKey> resolveKey(const Key& key, Backend& backend) {
    if (!key.valid()) return std::nullopt;

    ResolvedKey resolved;
    if (key.isScanCode()) {
        resolved.scan = key.scanCode();
        resolved.extended = key.extended();
        const auto scanEx = static_cast<std::uint16_t>(resolved.scan | (resolved.extended ? 0xE000 : 0));
        resolved.vk = backend.scanCodeToVk(scanEx);
        return resolved;
    }

    resolved.vk = key.vk();
    const std::uint16_t scanEx = backend.vkToScanCode(resolved.vk);
    const std::uint16_t prefix = scanEx >> 8;
    resolved.scan = scanEx & 0xFF;
    resolved.extended = prefix == 0xE0 || isExtendedVk(resolved.vk);

    if (resolved.vk == VK_SNAPSHOT) {
        // MapVirtualKeyEx reports 0x54 (Alt+SysRq); Print Screen itself is E0 37.
        resolved.scan = 0x37;
        resolved.extended = true;
    }
    if (prefix == 0xE1) resolved.scanUsable = false; // Pause: a multi-byte sequence
    return resolved;
}

std::optional<INPUT> makeKeyInput(const Key& key, KeyMode mode, bool up, Backend& backend) {
    const auto resolved = resolveKey(key, backend);
    if (!resolved) return std::nullopt;

    const bool canUseScan = resolved->scan != 0 && resolved->scanUsable;
    const bool canUseVk = resolved->vk != 0;
    if (!canUseScan && !canUseVk) return std::nullopt;

    const bool sendAsScan = mode == KeyMode::ScanCode ? canUseScan : !canUseVk;

    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = resolved->scanUsable ? resolved->scan : 0;
    if (sendAsScan) {
        input.ki.dwFlags = KEYEVENTF_SCANCODE;
    } else {
        input.ki.wVk = resolved->vk;
    }
    if (resolved->extended) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    if (up) input.ki.dwFlags |= KEYEVENTF_KEYUP;
    return input;
}

INPUT makeAbsoluteMove(Point target, const Rect& screen) {
    const Point p = clampToScreen(target, screen);
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = toAbsolute(p.x, screen.left, screen.width);
    input.mi.dy = toAbsolute(p.y, screen.top, screen.height);
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    return input;
}

INPUT makeRelativeMove(int dx, int dy) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    return input;
}

INPUT makeButtonInput(MouseButton button, bool up) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    switch (button) {
    case MouseButton::Left: input.mi.dwFlags = up ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN; break;
    case MouseButton::Right: input.mi.dwFlags = up ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN; break;
    case MouseButton::Middle: input.mi.dwFlags = up ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN; break;
    case MouseButton::X1:
    case MouseButton::X2:
        input.mi.dwFlags = up ? MOUSEEVENTF_XUP : MOUSEEVENTF_XDOWN;
        input.mi.mouseData = button == MouseButton::X1 ? XBUTTON1 : XBUTTON2;
        break;
    }
    return input;
}

INPUT makeWheelInput(int delta, bool horizontal) {
    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = horizontal ? MOUSEEVENTF_HWHEEL : MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(delta);
    return input;
}

std::optional<int> wheelDelta(double notches) {
    if (!std::isfinite(notches)) return std::nullopt;
    constexpr double kLimit = 1e6; // notches; keeps the delta inside an int
    return static_cast<int>(std::llround(std::clamp(notches, -kLimit, kLimit) * WHEEL_DELTA));
}

} // namespace inpututil::detail
