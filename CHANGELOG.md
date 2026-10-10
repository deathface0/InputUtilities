# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-10-10

Complete rewrite with a new API in the `inpututil` namespace; not source-compatible with 1.x.
See [Migrating from v1](README.md#migrating-from-v1) for the replacement of every v1 function.

### Added

- `Input` entry point with `input.keyboard` and `input.mouse`, configured through `Config`.
- `Key` with named constants, `Key::fromVk`, `Key::fromScanCode` and `Key::parse`; `KeyCombo` and combos as text (`press("Ctrl+Shift+Esc")`).
- `KeyMode` to send keys by virtual key or by scan code, with the extended-key flag set where needed.
- Text typing from UTF-8 or UTF-16, as Unicode or as real keystrokes of the active layout (Shift, AltGr, Caps Lock, dead keys), with delay and jitter.
- Exact absolute cursor placement on every monitor of the virtual desktop, including negative coordinates.
- Duration-based `Motion` (`instant`, `linear`, `smooth`, `human`) with easing and curved, human-like paths.
- `mouse.moveRaw` for relative movement in games that read raw input.
- Click options (count, hold, interval), a safe double click, drag and fractional scrolling, vertical and horizontal.
- RAII `Hold` for keys, combos and mouse buttons.
- `Sequence` macros as reusable values, validated up front and sent in atomic batches by `input.play`; `input.sendRaw` for raw `INPUT` events.
- Abort key (`Config::abortKey`) and `Input::requestAbort` to stop long operations, release everything and return `Error::Aborted`.
- `installEmergencyRelease` to release held input on crashes, `abort`, `std::terminate`, `std::exit` and console close.
- `Status` with the Win32 error code, and `Progress`, which also tells how many characters `type` typed and which step of `play` failed.
- Replaceable `Backend` for tests and custom injection.
- CMake build with presets, install and `find_package(inpututil)`, doctest unit tests, runnable examples and a guided manual check.
- GitHub Actions CI: MSVC (Debug and Release) and MinGW-w64 GCC with warnings as errors, an installed-package build and a clang-format check.

### Changed

- Every key and button pressed is tracked and released on destruction, by `releaseAll()` and by the abort key; this replaces safemode.
- Times are `std::chrono` durations instead of `int` milliseconds.
- Smooth movements take a duration instead of steps and a delay, and the duration is met.
- Reaching a target the cursor is already on is a success; `Config::verifyCursor` reports `TargetNotReached` when a target is missed.
- Requires C++20, CMake 3.21 and Windows 10; built as a static library.

### Removed

- The v1 API: `InputUtilities`, `InputUtilitiesCore`, `Event`, `InputType`, `InputResult`, `MWheelDir` and `MWheelAxis`.

### Fixed

- Absolute moves could land one pixel short of the target and only covered the primary monitor.
- Scan-code typing did not press Shift or AltGr, so `PlayerOne_123` came out as `playerone-123`.

[2.0.0]: https://github.com/deathface0/InputUtilities/releases/tag/v2.0.0
