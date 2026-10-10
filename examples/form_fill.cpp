// Fills a text field the way a person would: a curved mouse movement, a click,
// then typing with an irregular rhythm through the real keys of the layout.
#include "ExampleSupport.h"

using namespace inpututil;
using namespace std::chrono_literals;

int main() {
    examples::setUp();
    installEmergencyRelease(); // nothing stays pressed even if this program dies

    Input input({.abortKey = Key::F12});
    std::puts("Press F12 at any time to stop.");
    examples::countdown("Hover the window with the text field", 3);

    examples::report(input.mouse.moveTo({800, 400}, Motion::human(400ms)), "move to the field");
    examples::report(input.mouse.click(), "click");
    examples::report(
        input.keyboard.type("PlayerOne_123", {.mode = TextMode::Keystrokes, .delay = 70ms, .jitter = 30ms}),
        "type the user name");
    examples::report(input.keyboard.tap(Key::Enter), "press Enter");
    return 0;
}
