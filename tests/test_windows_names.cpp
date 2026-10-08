// Regression test: the public names must keep working in code that includes
// <windows.h> first and brings the whole namespace in. A Win32 function or
// macro with the same name as a public type (wingdi.h declares Chord(), for
// instance) would make this file fail to compile.
#include <windows.h>

#include "TestSupport.h"

#include <inpututil/inpututil.h>

using namespace inpututil;

TEST_CASE("public types are usable next to windows.h with using namespace") {
    const KeyCombo combo{{Key::Ctrl, Key::C}};
    const Key key = Key::A;
    const KeyMode mode = KeyMode::ScanCode;
    const Status status = Error::None;
    const Point point{1, 2};
    const Rect rect{0, 0, 10, 10};
    const Backend* backend = nullptr;
    const Config config{.keyMode = KeyMode::ScanCode};
    const Hold hold;
    const Input* input = nullptr;
    const Keyboard* keyboard = nullptr;
    const TypeOptions options{.mode = TextMode::Keystrokes};
    const Motion motion = Motion::human(std::chrono::milliseconds(300));
    const Easing easing = Easing::Linear;
    const MouseButton button = MouseButton::X1;
    const Mouse* mouse = nullptr;
    const ClickOptions click{.count = 2};

    CHECK(combo.keys.size() == 2);
    CHECK(key.valid());
    CHECK(mode == KeyMode::ScanCode);
    CHECK(status.ok());
    CHECK(rect.contains(point));
    CHECK(backend == nullptr);
    CHECK(config.releaseOnDestroy);
    CHECK_FALSE(hold.active());
    CHECK(input == nullptr);
    CHECK(keyboard == nullptr);
    CHECK(options.mode == TextMode::Keystrokes);
    CHECK(motion.curved);
    CHECK(easing == Easing::Linear);
    CHECK(button == MouseButton::X1);
    CHECK(mouse == nullptr);
    CHECK(click.count == 2);
}
