#include "inpututil/Backend.h"

#include <windows.h>

// timeapi.h needs windows.h first
#include <timeapi.h>

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

namespace inpututil {

namespace {

// Keyboard layout of the window that will receive the input; falls back to
// the calling thread's layout when there is no foreground window.
HKL foregroundLayout() {
    if (HWND window = GetForegroundWindow()) {
        if (DWORD thread = GetWindowThreadProcessId(window, nullptr)) return GetKeyboardLayout(thread);
    }
    return GetKeyboardLayout(0);
}

// One high-resolution waitable timer per thread (Windows 10 1803+).
class ThreadTimer {
public:
    ThreadTimer()
        : handle_(CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                         TIMER_ALL_ACCESS)) {}
    ~ThreadTimer() {
        if (handle_) CloseHandle(handle_);
    }
    ThreadTimer(const ThreadTimer&) = delete;
    ThreadTimer& operator=(const ThreadTimer&) = delete;

    // Waits for the given duration; false if high-resolution timers are unavailable.
    bool wait(std::chrono::nanoseconds duration) const {
        if (!handle_) return false;
        LARGE_INTEGER due;
        due.QuadPart = -static_cast<LONGLONG>(duration.count() / 100); // relative, 100 ns units
        if (due.QuadPart == 0) due.QuadPart = -1;
        if (!SetWaitableTimerEx(handle_, &due, 0, nullptr, nullptr, nullptr, 0)) return false;
        return WaitForSingleObject(handle_, INFINITE) == WAIT_OBJECT_0;
    }

private:
    HANDLE handle_;
};

class Win32Backend final : public Backend {
public:
    unsigned sendInput(std::span<const tagINPUT> inputs) override {
        if (inputs.empty()) return 0;
        return SendInput(static_cast<UINT>(inputs.size()), const_cast<INPUT*>(inputs.data()), sizeof(INPUT));
    }

    std::uint32_t lastError() override { return GetLastError(); }

    std::optional<Point> cursorPos() override {
        POINT p;
        if (!GetCursorPos(&p)) return std::nullopt;
        return Point{p.x, p.y};
    }

    Rect virtualScreen() override {
        return {GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN)};
    }

    std::int16_t vkKeyScan(wchar_t ch) override { return VkKeyScanExW(ch, foregroundLayout()); }

    std::uint16_t vkToScanCode(std::uint16_t vk) override {
        return static_cast<std::uint16_t>(MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC_EX, foregroundLayout()));
    }

    std::uint16_t scanCodeToVk(std::uint16_t scanCode) override {
        return static_cast<std::uint16_t>(MapVirtualKeyExW(scanCode, MAPVK_VSC_TO_VK_EX, foregroundLayout()));
    }

    bool isDeadKey(std::uint16_t vk, unsigned modifiers) override {
        BYTE state[256] = {};
        const auto press = [&state](int generic, int specific) { state[generic] = state[specific] = 0x80; };
        if (modifiers & 1) press(VK_SHIFT, VK_LSHIFT);
        if (modifiers & 2) press(VK_CONTROL, VK_LCONTROL);
        if (modifiers & 4) press(VK_MENU, VK_RMENU); // with Ctrl: AltGr

        const HKL layout = foregroundLayout();
        const UINT scan = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC, layout);
        wchar_t buffer[8];
        // Flag 0x4 (Windows 10 1607+) leaves the kernel's dead-key state alone,
        // so a dead key the user is typing at the same time is not disturbed.
        return ToUnicodeEx(vk, scan, state, buffer, 8, 0x4, layout) < 0;
    }

    bool isKeyDown(std::uint16_t vk) override { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

    bool isKeyToggled(std::uint16_t vk) override { return (GetKeyState(vk) & 1) != 0; }

    Clock::time_point now() override { return Clock::now(); }

    void sleepUntil(Clock::time_point deadline) override {
        thread_local const ThreadTimer timer;
        for (auto remaining = deadline - Clock::now(); remaining > Clock::duration::zero();
             remaining = deadline - Clock::now()) {
            if (timer.wait(remaining)) continue;

            // Fallback: raise the timer resolution only for the duration of the wait.
            const auto ms = std::chrono::ceil<std::chrono::milliseconds>(remaining).count();
            timeBeginPeriod(1);
            Sleep(static_cast<DWORD>(ms));
            timeEndPeriod(1);
        }
    }

    std::chrono::milliseconds doubleClickTime() override {
        return std::chrono::milliseconds(GetDoubleClickTime());
    }
};

} // namespace

std::shared_ptr<Backend> win32Backend() {
    static const auto instance = std::make_shared<Win32Backend>();
    return instance;
}

} // namespace inpututil
