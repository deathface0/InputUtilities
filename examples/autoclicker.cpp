// Clicks the left button repeatedly where the cursor is.
// Usage: inpututil_autoclicker [count=20] [interval_ms=100]
#include "ExampleSupport.h"

#include <cstdlib>

using namespace inpututil;

int main(int argc, char** argv) {
    examples::setUp();
    installEmergencyRelease();

    const int count = argc > 1 ? std::atoi(argv[1]) : 20;
    const int intervalMs = argc > 2 ? std::atoi(argv[2]) : 100;

    Input input({.abortKey = Key::F12});
    std::printf("%d clicks every %d ms. Press F12 to stop.\n", count, intervalMs);
    examples::countdown("Hover the target", 3);

    const Status status = input.mouse.click(
        MouseButton::Left, {.count = count, .interval = std::chrono::milliseconds(intervalMs)});
    examples::report(status, status == Error::Aborted ? "stopped with F12" : "clicks");
    return status ? 0 : 1;
}
