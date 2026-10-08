// A macro as a value: built once, played several times. Copies the selected
// text, clicks a second place and pastes it, then moves on to a new line.
#include "ExampleSupport.h"

using namespace inpututil;
using namespace std::chrono_literals;

int main() {
    examples::setUp();
    installEmergencyRelease();

    const Sequence copyPaste = Sequence{}
                                   .press("Ctrl+C")
                                   .moveTo({900, 500}, Motion::smooth(250ms))
                                   .click()
                                   .press("Ctrl+V")
                                   .wait(100ms)
                                   .type("\n");

    Input input({.abortKey = Key::F12});
    std::printf("A %zu-step macro played 3 times. Press F12 to stop.\n", copyPaste.size());
    examples::countdown("Select some text in an editor", 3);

    for (int round = 1; round <= 3; ++round) {
        if (!examples::report(input.play(copyPaste), "play")) break;
        std::this_thread::sleep_for(500ms);
    }
    return 0;
}
