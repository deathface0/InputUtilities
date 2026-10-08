// Regression test: the public names must keep working in code that includes
// <windows.h> first and brings the whole namespace in. A Win32 function or
// macro with the same name as a public type (wingdi.h declares Chord(), for
// instance) would make this file fail to compile.
#include <windows.h>

#include "TestSupport.h"

#include <inpututil/Backend.h>
#include <inpututil/Key.h>
#include <inpututil/Point.h>
#include <inpututil/Status.h>

using namespace inpututil;

TEST_CASE("public types are usable next to windows.h with using namespace") {
    const KeyCombo combo{{Key::Ctrl, Key::C}};
    const Key key = Key::A;
    const KeyMode mode = KeyMode::ScanCode;
    const Status status = Error::None;
    const Point point{1, 2};
    const Rect rect{0, 0, 10, 10};
    const Backend* backend = nullptr;

    CHECK(combo.keys.size() == 2);
    CHECK(key.valid());
    CHECK(mode == KeyMode::ScanCode);
    CHECK(status.ok());
    CHECK(rect.contains(point));
    CHECK(backend == nullptr);
}
