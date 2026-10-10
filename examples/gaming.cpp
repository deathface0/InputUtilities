// Game input: scan codes (what DirectInput / raw input games read), holding a
// key while doing other things, and raw relative mouse movement to turn the
// camera (absolute moves are ignored by most first-person games).
#include "ExampleSupport.h"

using namespace inpututil;
using namespace std::chrono_literals;

int main() {
    examples::setUp();
    installEmergencyRelease();

    Input input({.keyMode = KeyMode::ScanCode, .abortKey = Key::F12});
    std::puts("Press F12 at any time to stop.");
    examples::countdown("Focus the game window", 3);

    {
        // Run forward: W stays down until `run` goes out of scope.
        auto run = input.keyboard.hold(Key::W);
        examples::report(run.status(), "hold W");

        // While running: side button (e.g. melee) and a smooth 400-mickey camera turn.
        examples::report(input.mouse.click(MouseButton::X1, {.hold = 50ms}), "side button");
        examples::report(input.mouse.moveRaw(400, 0, Motion::linear(300ms)), "turn the camera");

        std::this_thread::sleep_for(1s);
    } // W released here

    examples::report(input.keyboard.tap(Key::Space), "jump");
    return 0;
}
