#include "inpututil/EmergencyRelease.h"

#include <windows.h>

#include <csignal>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <vector>

#include "detail/Session.h"

namespace inpututil {

namespace {

struct Registry {
    std::timed_mutex mutex;
    std::vector<detail::Session*> sessions;
};

// Deliberately never destroyed: handlers may run while static objects are
// being torn down at the end of the program.
Registry& registry() {
    static auto* const instance = new Registry;
    return *instance;
}

using SignalHandler = void (*)(int);

LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter = nullptr;
SignalHandler previousAbortHandler = nullptr;
std::terminate_handler previousTerminateHandler = nullptr;

LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* info) {
    emergencyReleaseAll();
    return previousExceptionFilter ? previousExceptionFilter(info) : EXCEPTION_CONTINUE_SEARCH;
}

void onAbortSignal(int signal) {
    emergencyReleaseAll();
    if (previousAbortHandler && previousAbortHandler != SIG_DFL && previousAbortHandler != SIG_IGN)
        previousAbortHandler(signal);
}

void onTerminate() {
    emergencyReleaseAll();
    if (previousTerminateHandler) previousTerminateHandler();
    std::abort();
}

void onExit() { emergencyReleaseAll(); }

BOOL WINAPI onConsoleEvent(DWORD) {
    emergencyReleaseAll();
    return FALSE; // let Windows go on (normally: end the process)
}

} // namespace

namespace detail {

void registerSession(Session* session) {
    Registry& r = registry();
    std::lock_guard lock(r.mutex);
    r.sessions.push_back(session);
}

void unregisterSession(Session* session) {
    Registry& r = registry();
    std::lock_guard lock(r.mutex);
    std::erase(r.sessions, session);
}

} // namespace detail

void emergencyReleaseAll() noexcept {
    try {
        Registry& r = registry();
        std::unique_lock lock(r.mutex, std::defer_lock);
        (void)lock.try_lock_for(std::chrono::milliseconds(100)); // go on even if it is stuck
        for (detail::Session* session : r.sessions) session->emergencyRelease();
    } catch (...) {
        // Nothing else can be done while the process dies.
    }
}

void installEmergencyRelease() {
    static std::once_flag installed;
    std::call_once(installed, [] {
        previousExceptionFilter = SetUnhandledExceptionFilter(&onUnhandledException);
        const SignalHandler previous = std::signal(SIGABRT, &onAbortSignal);
        previousAbortHandler = previous == SIG_ERR ? nullptr : previous;
        previousTerminateHandler = std::set_terminate(&onTerminate);
        std::atexit(&onExit);
        SetConsoleCtrlHandler(&onConsoleEvent, TRUE);
    });
}

} // namespace inpututil
