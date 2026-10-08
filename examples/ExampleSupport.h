#pragma once

// Small helpers shared by the examples. Not part of the library.
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <string_view>
#include <thread>

#include <inpututil/inpututil.h>

namespace examples {

/// Makes the process per-monitor DPI aware so screen coordinates are real
/// pixels even on scaled displays (125%, 150%...). Without it Windows
/// virtualizes coordinates for the process. Looked up at run time so it also
/// builds with older SDK headers.
inline void enableDpiAwareness() {
    using SetContext = BOOL(WINAPI*)(HANDLE);
    if (const HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        if (const auto set = reinterpret_cast<SetContext>(
                reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")))) {
            set(reinterpret_cast<HANDLE>(-4)); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        }
    }
}

/// Console setup shared by every example: UTF-8 output and DPI awareness.
inline void setUp() {
    SetConsoleOutputCP(CP_UTF8);
    enableDpiAwareness();
}

/// Gives the user time to focus the target window.
inline void countdown(std::string_view what, int seconds) {
    std::printf("%.*s - starting in", static_cast<int>(what.size()), what.data());
    for (int i = seconds; i > 0; --i) {
        std::printf(" %d", i);
        std::fflush(stdout);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::printf("\n");
}

/// Prints the outcome of a step and returns whether it succeeded.
inline bool report(const inpututil::Status& status, std::string_view step) {
    std::printf("  %-28.*s %s\n", static_cast<int>(step.size()), step.data(),
                status ? "OK" : status.message().c_str());
    return status.ok();
}

} // namespace examples
