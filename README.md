
# InputUtilities 🖱️⌨️

A robust, high-performance C++ library for simulating Mouse and Keyboard input on Windows.

Designed for automation, game scripting, and macro creation, **InputUtilities** provides a safe wrapper around the Windows `SendInput` API. It features a unique **Safemode** system that tracks active inputs and automatically releases them upon destruction or error, preventing "stuck keys" or "stuck mouse buttons."

## ✨ Features

* **🛡️ Safemode (RAII Style):** Automatically tracks held keys and mouse buttons. If your program crashes, throws an exception, or goes out of scope, the destructor releases all active inputs immediately via a batch release.
* **🖱️ Advanced Mouse Control:**
    * Absolute and Relative movement.
    * **Smooth Mouse Movement:** Interpolated movement with Ease-in-out curves (human-like simulation).
    * Support for **Extra Buttons** (Side buttons XBUTTON1/XBUTTON2).
    * Scroll control (Vertical & Horizontal).
* **⌨️ Versatile Keyboard Input:**
    * **Virtual Keys (VK):** Standard Windows key codes.
    * **Scan Codes (SC):** Hardware-level simulation (essential for DirectX games).
    * **Unicode (UC):** Text injection support (supports special characters like Chinese, emojis, etc.).
    * **String Typing:** Helper functions to type full strings with natural delays.
* **⚡ High Performance:** Uses `SendInput` with minimal overhead.

## ⚙️ Performance & Safemode

One of the key features of this library is the toggleable **Safemode**.

### `false` (Default) — Maximum Performance
* **Behavior:** The library acts as a thin, zero-overhead wrapper around `SendInput`.
* **Use Case:** High-frequency clicking (e.g., 1000 clicks/sec), tight loops, or applications where CPU cycles are critical.
* **Risk:** If your application crashes while holding a key down (e.g., `Ctrl`), that key will remain physically "pressed" in Windows until you press it again manually.

### `true` — Safety First
* **Behavior:** Every input event is tracked in an internal `unordered_set` (Hash Map) using O(1) operations.
* **Use Case:** Complex macros, game bots, or long-running scripts where stability is priority.
* **Benefit:** If the object goes out of scope or the program terminates unexpectedly, `reset()` is called automatically to release all held keys.

```cpp
// 1. Performance Mode (Default) - Zero overhead
InputUtilities inputFast; 

// 2. Safemode - Tracks inputs, auto-releases on crash/exit
InputUtilities inputSafe(true); 
```

## 📦 Installation

This is a header-only compatible library structure.

1. Copy the following files into your C++ project source folder:
* `InputUtilities.h` / `.cpp`
* `InputUtilitiesCore.h` / `.cpp`
* `InputData.h`


2. Include `InputUtilities.h` in your main file.
3. Ensure you link against `User32.lib` (Standard in Visual Studio).

## 🚀 Usage Examples

### 1. The "Human-Like" Simulation

This example combines smooth mouse movement with delayed typing to mimic a real user filling out a form.

```cpp
#include "InputUtilities.h"

int main() {
    InputUtilities input(true); // Safemode ON recommended for complex scripts

    // 1. Move mouse smoothly to coordinates (800, 400)
    //    Take 20 steps, with 5ms delay between steps.
    input.SetCursorPos(800, 400, 20, 5, true);

    // 2. Click the text box (50ms hold time)
    input.leftClick(50); 

    // 3. Type a username with natural variance
    //    75ms delay between keystrokes makes it look human.
    input.typeStr(L"PlayerOne_123", 75);

    // 4. Press Enter
    input.vKey(VK_RETURN, 50);

    return 0;
}

```

### 2. Gamer Mode (Hardware Scan Codes)

Many DirectX games (FPS/MMO) ignore standard Windows keys. This example uses **Scan Codes** (`scKey`) which simulate hardware interrupts, and interacts with Side Mouse Buttons.

```cpp
#include "InputUtilities.h"

int main() {
    InputUtilities input(true);

    // Press 'W' using Hardware Scan Code (Simulates physical keyboard)
    // Hold for 2 seconds (Running forward)
    input.scKey(L'w', 2000); 

    // While running, press Mouse Side Button 1 (e.g., Melee attack)
    input.extraClick(XBUTTON1, 100); 

    // Press Spacebar (Jump) via Scan Code
    input.scKey(L' ', 50); 

    return 0;
}

```

### 3. High-Speed Auto Clicker

For scenarios requiring raw speed, disable Safemode to skip the tracking logic overhead.

```cpp
#include "InputUtilities.h"

int main() {
    // Disable Safemode for maximum throughput
    InputUtilities input(false); 

    // Spam Left Click as fast as the OS can handle
    for(int i = 0; i < 1000; ++i) {
        input.leftClick(0); // 0ms hold time = Instant tap
        Sleep(10); // Optional regulation
    }

    return 0;
}

```

### 4. Advanced: Manual "Hold" Logic & RAII

Sometimes you need to hold a key while performing other actions. This demonstrates how Safemode saves you from stuck keys.

```cpp
#include "InputUtilities.h"

int main() {
    {
        InputUtilities input(true);

        // 1. Hold 'Shift' (e.g., to select text or sprint)
        input.vKeyDown(VK_SHIFT);

        // 2. Perform other actions while Shift is held
        input.vKey(VK_RIGHT, 50);
        
        // ... Imagine an exception occurs here or the program crashes ...
        
        // Because Safemode is ON, the destructor is called here.
        // It detects VK_SHIFT is still down and releases it automatically.
    }
    
    return 0;
}

```

## 🛠️ API Overview

### `InputUtilities` (High-Level Wrapper)

* `leftClick(ms)`, `rightClick(ms)`, `middleClick(ms)`
* `extraClick(button, ms)`: Supports `XBUTTON1` and `XBUTTON2`.
* `vKey(code, ms)`: Standard Virtual Key press.
* `scKey(char, ms)`: Scan Code press (Hardware simulation).
* `unicodeKey(char, ms)`: Unicode character injection.
* `typeStr(string, delay)`: Types a full string using Unicode.
* `scTypeStr(string, delay)`: Types a full string using Scan Codes.

### `InputUtilitiesCore` (Low-Level Control)

* `SetCursorPos(x, y, abs)`: Instant movement.
* `SetCursorPos(x, y, steps, delay, abs)`: Smooth interpolated movement.
* `MouseEvent(flags, data)`: Raw mouse event injection.
* `vKeyDown/Up`, `scKeyDown/Up`, `unicodeKeyDown/Up`: Manual state control.
* `MouseWheelRoll(scrolls, direction, ...)`: Precise scrolling control.

## 📋 Requirements

* **OS:** Windows (Uses `WinUser.h`)
* **Compiler:** MSVC (Visual Studio) or MinGW with C++17 support recommended.

## 📄 License

This project is open-source. Feel free to use it in your personal or commercial projects.