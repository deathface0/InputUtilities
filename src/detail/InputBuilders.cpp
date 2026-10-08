#include "detail/InputBuilders.h"

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

} // namespace inpututil::detail
