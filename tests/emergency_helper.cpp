// Helper process for the emergency-release tests: presses a key through a
// backend that prints instead of injecting input, then dies on purpose in the
// way given on the command line. The CTest entries check that the release
// was printed before the process went away.
#include <windows.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

#include "FakeBackend.h"

#include <inpututil/inpututil.h>

namespace {

class PrintingBackend : public FakeBackend {
public:
    unsigned sendInput(std::span<const tagINPUT> inputs) override {
        for (const INPUT& in : inputs) {
            if (in.type != INPUT_KEYBOARD) continue;
            const bool up = (in.ki.dwFlags & KEYEVENTF_KEYUP) != 0;
            std::printf("%s vk=0x%02X\n", up ? "RELEASED" : "DOWN", in.ki.wVk);
        }
        std::fflush(stdout);
        return FakeBackend::sendInput(inputs);
    }
};

// No crash dialogs, debugger prompts or error reports: the tests run unattended.
void silenceCrashReporting() {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS*) -> LONG {
        std::fflush(stdout);
        TerminateProcess(GetCurrentProcess(), 99);
        return EXCEPTION_EXECUTE_HANDLER;
    });
    // abort(): end with a normal exit code instead of the CRT's fast-fail
    // (which CTest reports as a crash). The library chains to this handler.
    std::signal(SIGABRT, [](int) {
        std::fflush(stdout);
        std::_Exit(98);
    });
#ifdef _MSC_VER
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    const std::string_view mode = argv[1];

    silenceCrashReporting(); // installed first, so the library chains to it
    inpututil::installEmergencyRelease();

    inpututil::Input input(
        inpututil::Config{.releaseOnDestroy = false, .backend = std::make_shared<PrintingBackend>()});
    if (!input.keyboard.down(inpututil::Key::A)) return 3;

    if (mode == "crash") {
        volatile int* nowhere = nullptr;
        *nowhere = 42;
    } else if (mode == "terminate") {
        throw std::runtime_error("nobody catches this");
    } else if (mode == "abort") {
        std::abort();
    } else if (mode == "exit") {
        std::exit(0); // locals, including `input`, are not destroyed
    }

    std::puts("STILL ALIVE");
    return 1;
}
