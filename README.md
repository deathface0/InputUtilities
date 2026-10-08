# InputUtilities

[![CI](https://github.com/deathface0/InputUtilities/actions/workflows/ci.yml/badge.svg)](https://github.com/deathface0/InputUtilities/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Mouse and keyboard input simulation for Windows (`SendInput`), with exact cursor placement and nothing left pressed.

```cpp
#include <cstdio>
#include <inpututil/inpututil.h>

using namespace inpututil;
using namespace std::chrono_literals;

int main() {
    Input input({.abortKey = Key::F12});      // F12 stops any long operation

    input.mouse.moveTo({800, 400}, Motion::human(350ms));
    input.mouse.click();
    input.keyboard.type("Hello, ñandú 😀", {.delay = 40ms, .jitter = 20ms});
    input.keyboard.press("Ctrl+S");

    if (Status st = input.keyboard.press("Ctrl+Shift+Esc"); !st)
        std::printf("failed: %s\n", st.message().c_str());
}
```

## Features

- **Keyboard** by virtual key or scan code (`KeyMode`), with the extended-key flag set correctly for arrows, Insert, Delete, Home, End, PageUp/Down, right Ctrl/Alt…
- **Named keys and combos**: `Key::F5`, `Key::parse("PgUp")`, `press("Ctrl+Shift+Esc")`.
- **Text** as Unicode (any character, emoji included) or as real keystrokes of the active layout, with Shift, AltGr and Caps Lock handled.
- **Exact cursor placement** on every monitor of the virtual desktop, including negative coordinates.
- **Duration-based motion** with easing and curved, human-like paths; the duration is actually met.
- **Raw relative movement** (`moveRaw`) to turn the camera in games that ignore absolute moves.
- **Nothing left pressed**: every key and button is tracked, released on destruction, by RAII `Hold`s, by the abort key and, optionally, when the process crashes or the console is closed.
- **Sequences**: macros as values, sent in atomic batches and validated before anything is sent.
- **Testable**: all system access goes through a `Backend` you can replace.
- Builds with **0 warnings** under MSVC (`/W4 /WX`) and MinGW-w64 GCC (`-Wall -Wextra -Wpedantic -Werror`), checked in CI.

## Installation

Requirements: Windows 10 or later, a C++20 compiler (MSVC 2022 or MinGW-w64 GCC, both tested in CI) and CMake 3.21+.

**FetchContent**

```cmake
include(FetchContent)
FetchContent_Declare(inpututil
    GIT_REPOSITORY https://github.com/deathface0/InputUtilities.git
    GIT_TAG v2.0.0)
FetchContent_MakeAvailable(inpututil)

target_link_libraries(my_app PRIVATE inpututil::inpututil)
```

**Subdirectory**: copy or submodule the repository, then `add_subdirectory(InputUtilities)` and link `inpututil::inpututil`.

**Installed package**: `cmake --install` the library, then `find_package(inpututil 2 REQUIRED)` and link `inpututil::inpututil`.

When used as a subproject, the tests and examples are not built.

## Guide

### Input and Config

`Input` is the entry point. It owns a keyboard and a mouse, tracks everything it presses and releases it when destroyed. It cannot be copied; it can be moved.

```cpp
Input defaults;                                // Win32, virtual keys, release on destroy
Input game({
    .keyMode = KeyMode::ScanCode,              // how keys are injected (see below)
    .releaseOnDestroy = true,                  // release held keys/buttons in the destructor
    .verifyCursor = true,                      // moveTo/moveBy fail with TargetNotReached if missed
    .abortKey = Key::F12,                      // stops long operations
});
game.releaseAll();                             // release everything held right now
std::printf("%zu keys/buttons held\n", game.heldCount());
```

### Keyboard

```cpp
input.keyboard.tap(Key::Enter);
input.keyboard.tap(Key::A, 50ms);              // keep it down for 50 ms
input.keyboard.press("Ctrl+C");                // pressed in order, released in reverse, one batch
input.keyboard.press(KeyCombo{{Key::Alt, Key::F4}});
input.keyboard.down(Key::Shift);
input.keyboard.up(Key::Shift);
input.keyboard.setMode(KeyMode::ScanCode);
```

**Keys.** Named constants (`Key::A`…`Key::Z`, `Key::Digit0`…`Key::Digit9`, `Key::F1`…`Key::F24`, `Key::Ctrl`, `Key::LShift`, `Key::AltGr`, `Key::Left`, `Key::NumpadEnter`, media keys…), any virtual key with `Key::fromVk(vk)`, or a physical position with `Key::fromScanCode(sc)` (WASD on any layout). `Key::parse("pgup")` and `KeyCombo::parse("Ctrl+Shift+Esc")` read names case-insensitively and accept aliases; `key.name()` and `combo.toString()` print them back.

**Key mode.** `KeyMode::VirtualKey` (default) sends the virtual key together with its scan code and extended flag, which is what applications expect. `KeyMode::ScanCode` sends `KEYEVENTF_SCANCODE`, which DirectInput and raw input games usually need.

### Text

```cpp
input.keyboard.type("Any text: ñ € 😀\n");     // Unicode, layout independent
input.keyboard.type(L"PlayerOne_123", {
    .mode = TextMode::Keystrokes,              // real keys of the foreground window's layout
    .delay = 60ms,
    .jitter = 25ms,                            // random ± variation of each pause
});
```

UTF-8 (`std::string_view`, `u8""`) and UTF-16 (`std::wstring_view`) are accepted. `\n`, `\r\n` and `\t` press Enter and Tab. In `Keystrokes` mode, characters without a key (and emoji) fall back to Unicode unless `.fallbackToUnicode = false`, in which case the call fails with `UnmappableCharacter` before typing anything. Invalid text fails with `InvalidArgument`, also before anything is sent.

### Mouse

```cpp
if (std::optional<Point> where = input.mouse.position())
    std::printf("cursor at %d, %d\n", where->x, where->y);
input.mouse.moveTo({1000, 500});                           // instant, exact pixel
input.mouse.moveTo({-1200, 300}, Motion::smooth(250ms));   // a monitor left of the primary one
input.mouse.moveBy(50, -20, Motion::human(200ms));
input.mouse.moveRaw(400, 0, Motion::linear(300ms));        // relative mickeys, for game cameras

input.mouse.click();
input.mouse.click(MouseButton::Right, {.count = 3, .interval = 80ms});
input.mouse.doubleClick();
input.mouse.drag({100, 100}, {600, 400}, Motion::human(400ms));
input.mouse.scroll(3);                                     // up; negative scrolls down
input.mouse.scroll(-2.5, 200ms);                           // fractions and spreading over time
input.mouse.scrollHorizontal(1);                           // right
```

**Motion.** A movement is described by its duration, not by steps: `Motion::instant()`, `Motion::linear(d)`, `Motion::smooth(d)`, `Motion::human(d)` (curved path, natural acceleration, one pixel of jitter), or your own:

```cpp
const Motion custom{
    .duration = 300ms,
    .easing = Easing::EaseInOutCubic,          // Linear, SmoothStep, EaseInOutCubic
    .curved = true,                            // random Bézier curve instead of a straight line
    .jitterPx = 2,                             // random offset of intermediate points
    .stepInterval = 4ms,                       // time between points
    .seed = 42,                                // reproducible path (0 = random)
};
input.mouse.moveTo({500, 500}, custom);
```

Each point is sent at its own deadline, so late wake-ups never add up and the movement takes the requested time.

### Holding keys and buttons

```cpp
{
    auto shift = input.keyboard.hold(Key::Shift);
    auto drag = input.mouse.hold(MouseButton::Left);
    input.mouse.moveBy(200, 0, Motion::smooth(300ms));
} // both released here, last pressed first
```

A `Hold` releases when it is destroyed, moved over or `release()` is called. It is safe even if it outlives its `Input`.

### Sequences

```cpp
const Sequence copyPaste = Sequence{}
    .press("Ctrl+C")
    .moveTo({900, 500}, Motion::smooth(250ms))
    .click()
    .press("Ctrl+V")
    .wait(100ms)
    .type("done\n");

input.play(copyPaste);                         // can be played again and again
```

Consecutive instant steps go in one `SendInput` call. Every step is validated first: a bad combo or invalid text anywhere fails `play` before anything is sent. If a step fails midway, whatever the sequence pressed is released (your own `Hold`s are left alone).

### Errors

Every operation returns a `Status`, which converts to `true` on success:

```cpp
if (Status st = input.mouse.moveTo({100, 100}); !st) {
    // st.error(): SystemFailure, PartialSend, InvalidArgument,
    //             UnmappableCharacter, TargetNotReached or Aborted
    if (st.error() == Error::SystemFailure)
        std::printf("GetLastError() = %u\n", st.win32Error());
    std::printf("%s\n", st.message().c_str()); // e.g. "SystemFailure (Win32 error 5: Access is denied.)"
}
```

No exceptions are thrown.

### Stopping a runaway macro

With `Config::abortKey`, every operation that takes time (motions, holds, typing, timed clicks and scrolls, sequence waits) checks the key every 10 ms. When it goes down, everything held is released and the operation returns `Error::Aborted`. Instant actions are not affected. It can be changed at any time with `input.setAbortKey(...)`.

### Emergency release

```cpp
int main() {
    installEmergencyRelease();                 // once, at the start
    Input input;
    // ...
}
```

Destructors do not run when a process crashes, calls `abort()` or `std::exit()`, has an uncaught exception, or its console is closed. `installEmergencyRelease()` installs handlers for all of those cases that release everything held by every `Input`. Existing handlers are chained, not replaced. `emergencyReleaseAll()` does the same on demand.

### Advanced: custom backend and raw events

All system access (injection, cursor, screen, keyboard layout, key state and time) goes through `Backend`. Pass your own implementation in `Config::backend`, for example to drive a kernel-level injector or to record events in tests. The library's own tests use a fake backend with a simulated clock.

`input.sendRaw(events)` sends raw Win32 `INPUT` structures (include `<windows.h>` to build them) in one batch. Keys and buttons pressed this way are tracked like any other.

## Safety and limitations

- **What is released, and when.** Everything held is released by `releaseAll()`, by the destructor (unless `releaseOnDestroy = false`), when a `Hold` ends, when the abort key is pressed and, if `installEmergencyRelease()` was called, on crashes, `abort`, `std::terminate`, `std::exit` and console close. **Nothing can run when the process is killed** (Task Manager, `taskkill /F`, `TerminateProcess`); use the abort key to stop a macro while it is still running.
- **Elevated windows (UIPI).** Windows silently drops input sent to a process with higher integrity (for example one running as administrator) and `SendInput` reports no error. Run your program at the same level as the target.
- **Injected input is detectable.** Every event sent through `SendInput` carries the `LLKHF_INJECTED` / `LLMHF_INJECTED` flag, whether it uses virtual keys or scan codes. Anti-cheat software can see it.
- **DPI scaling.** On scaled displays (125 %, 150 %…) Windows virtualizes coordinates for processes that are not DPI aware. Make your process per-monitor DPI aware to work with real pixels; see `enableDpiAwareness()` in [`examples/ExampleSupport.h`](examples/ExampleSupport.h).
- **`moveRaw`** moves in mickeys: the distance in pixels depends on the pointer speed and "Enhance pointer precision", so the final position is not verified.
- **`TextMode::Keystrokes`** uses the keyboard layout of the foreground window. If you hold a modifier yourself (for example with `hold(Key::Shift)`), it affects the typed text.
- **The abort key** is read with `GetAsyncKeyState`, which also sees injected keys: a macro that sends its own abort key stops itself.
- Every event carries `dwExtraInfo = kDefaultExtraInfoTag` (configurable with `Config::extraInfoTag`), so your own hooks can tell it apart from real input.

## Examples

| Example | Shows |
|---|---|
| [`form_fill.cpp`](examples/form_fill.cpp) | Human-like movement, click and typing with an irregular rhythm |
| [`gaming.cpp`](examples/gaming.cpp) | Scan codes, holding W while clicking a side button, raw camera turn |
| [`autoclicker.cpp`](examples/autoclicker.cpp) | Repeated clicks with an interval, stopped with F12 |
| [`macro_sequence.cpp`](examples/macro_sequence.cpp) | A reusable `Sequence` played several times |
| [`manual_check.cpp`](examples/manual_check.cpp) | Guided checks with the real mouse and keyboard: cursor accuracy on every monitor, typing, abort key, emergency release |

They move the real mouse and type into the focused window, so they are never run by the test suite.

## Building and testing

```bash
cmake --preset msvc                        # Visual Studio 2022 generator
cmake --build --preset msvc-debug
ctest --preset msvc-debug
```

Other presets: `msvc-ninja` (Ninja + MSVC, any Visual Studio version, run from a developer prompt; also generates `compile_commands.json`) and `mingw` (MinGW-w64 GCC). Pass `-DINPUTUTIL_WARNINGS_AS_ERRORS=ON` to treat warnings as errors, as CI does.

The unit tests use a fake backend and never send real input. Four extra tests start a helper process that dies on purpose (crash, `terminate`, `abort`, `exit`) to check the emergency release; its backend prints the events instead of sending them.

## Migrating from v1

Version 2 is a rewrite with a new API in the `inpututil` namespace. Everything v1 did has a replacement:

| v1 | v2 |
|---|---|
| `InputUtilities input(safemode)` | `inpututil::Input input;` (tracking is always on) |
| `leftClick(ms)`, `rightClick(ms)`, `middleClick(ms)` | `input.mouse.click(MouseButton::Left, {.hold = ms})`… |
| `extraClick(XBUTTON1, ms)` | `input.mouse.click(MouseButton::X1, {.hold = ms})` |
| `ExtraClickDown(b)` / `ExtraClickUp(b)` | `input.mouse.down(MouseButton::X1)` / `up(...)`, or `hold(...)` |
| `MouseEvent(flags, data)` | `input.mouse.down/up(...)`, or `input.sendRaw(...)` |
| `vKey(vk, ms)` | `input.keyboard.tap(Key::fromVk(vk), ms)` |
| `scKey(ch, ms)` | `input.keyboard.setMode(KeyMode::ScanCode)` + `tap(Key::...)`, or `type(text, {.mode = TextMode::Keystrokes})` for characters |
| `unicodeKey(ch, ms)` | `input.keyboard.type(text)` |
| `Key(Event, ms)` | `input.keyboard.tap(key, ms)` |
| `vKeyDown/Up`, `scKeyDown/Up`, `unicodeKeyDown/Up`, `keyDown/Up` | `input.keyboard.down(key)` / `up(key)`, or `hold(key)` |
| `vkMultiKey`, `scMultiKey`, `unicodeMultiKey`, `multiKey` | `input.keyboard.press("Ctrl+Shift+Esc")` or `press(KeyCombo{...})` |
| `*MultiKeyDown` / `*MultiKeyUp` | `input.keyboard.hold(KeyCombo{...})` |
| `typeStr(text, delay)` | `input.keyboard.type(text, {.delay = ...})` |
| `scTypeStr(text, delay)` | `input.keyboard.type(text, {.mode = TextMode::Keystrokes, .delay = ...})` |
| `SetCursorPos(x, y, abs)` | `input.mouse.moveTo({x, y})` / `moveBy(dx, dy)` |
| `SetCursorPos(x, y, steps, delay, abs)` | `input.mouse.moveTo({x, y}, Motion::smooth(duration))` |
| `MouseWheelRoll(n, dir, delta, axis)` | `input.mouse.scroll(±n)` / `scrollHorizontal(±n)` |
| `MouseWheelRoll(n, delay, dir, ...)` | `input.mouse.scroll(±n, duration)` |
| `Event{InputType::VK / SC / UC, code}` | `Key::fromVk(vk)` / `Key::fromScanCode(sc)`; Unicode characters go through `type(text)` |
| `InputResult` | `Status` / `Error` |
| `MWheelDir`, `MWheelAxis` | The sign of the notches; `scroll` vs `scrollHorizontal` |

Behaviour changes worth knowing:

- Times are `std::chrono` durations (`50ms`) instead of `int` milliseconds.
- Smooth movements take a duration instead of steps and delay, and that duration is met.
- `MouseSamePos` is gone: reaching a target the cursor was already on is a success. Enable `Config::verifyCursor` to get `TargetNotReached` when a target is missed.
- Scan-code typing now presses Shift/AltGr where the layout needs it (`scTypeStr(L"PlayerOne_123")` used to type `playerone-123`).
- Absolute coordinates cover all monitors and land on the exact pixel (v1 was limited to the primary monitor and could be one pixel short).

## License

[MIT](LICENSE)
