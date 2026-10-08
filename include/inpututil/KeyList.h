#pragma once

// The single list of named keys. Each entry is ENTRY(Name, expression), where
// the expression builds the Key inside the scope of class Key. Key.h expands
// it into the Key::Name constants, KeyNames.cpp into the name table, and the
// tests into the list of every named key, so a key is only ever written here.
//
// Values are the Win32 VK_* codes (written as numbers so no public header
// needs <windows.h>). Aliases of another constant (AltGr, Win) must come after
// it: Key::name() returns the first matching entry.
//
// Only /* */ comments inside the macro: a // comment would swallow the line
// continuation.

// clang-format off
#define INPUTUTIL_NAMED_KEYS(ENTRY)                                                      \
    /* Letters */                                                                        \
    ENTRY(A, fromVk(0x41)) ENTRY(B, fromVk(0x42)) ENTRY(C, fromVk(0x43))                 \
    ENTRY(D, fromVk(0x44)) ENTRY(E, fromVk(0x45)) ENTRY(F, fromVk(0x46))                 \
    ENTRY(G, fromVk(0x47)) ENTRY(H, fromVk(0x48)) ENTRY(I, fromVk(0x49))                 \
    ENTRY(J, fromVk(0x4A)) ENTRY(K, fromVk(0x4B)) ENTRY(L, fromVk(0x4C))                 \
    ENTRY(M, fromVk(0x4D)) ENTRY(N, fromVk(0x4E)) ENTRY(O, fromVk(0x4F))                 \
    ENTRY(P, fromVk(0x50)) ENTRY(Q, fromVk(0x51)) ENTRY(R, fromVk(0x52))                 \
    ENTRY(S, fromVk(0x53)) ENTRY(T, fromVk(0x54)) ENTRY(U, fromVk(0x55))                 \
    ENTRY(V, fromVk(0x56)) ENTRY(W, fromVk(0x57)) ENTRY(X, fromVk(0x58))                 \
    ENTRY(Y, fromVk(0x59)) ENTRY(Z, fromVk(0x5A))                                        \
    /* Digits of the main block */                                                       \
    ENTRY(Digit0, fromVk(0x30)) ENTRY(Digit1, fromVk(0x31)) ENTRY(Digit2, fromVk(0x32))  \
    ENTRY(Digit3, fromVk(0x33)) ENTRY(Digit4, fromVk(0x34)) ENTRY(Digit5, fromVk(0x35))  \
    ENTRY(Digit6, fromVk(0x36)) ENTRY(Digit7, fromVk(0x37)) ENTRY(Digit8, fromVk(0x38))  \
    ENTRY(Digit9, fromVk(0x39))                                                          \
    /* Numeric keypad (VK_NUMPAD0..9) */                                                 \
    ENTRY(Numpad0, fromVk(0x60)) ENTRY(Numpad1, fromVk(0x61))                            \
    ENTRY(Numpad2, fromVk(0x62)) ENTRY(Numpad3, fromVk(0x63))                            \
    ENTRY(Numpad4, fromVk(0x64)) ENTRY(Numpad5, fromVk(0x65))                            \
    ENTRY(Numpad6, fromVk(0x66)) ENTRY(Numpad7, fromVk(0x67))                            \
    ENTRY(Numpad8, fromVk(0x68)) ENTRY(Numpad9, fromVk(0x69))                            \
    ENTRY(NumpadMultiply, fromVk(0x6A))           /* VK_MULTIPLY */                      \
    ENTRY(NumpadAdd, fromVk(0x6B))                /* VK_ADD */                           \
    ENTRY(NumpadSubtract, fromVk(0x6D))           /* VK_SUBTRACT */                      \
    ENTRY(NumpadDecimal, fromVk(0x6E))            /* VK_DECIMAL */                       \
    ENTRY(NumpadDivide, fromVk(0x6F))             /* VK_DIVIDE */                        \
    ENTRY(NumpadEnter, fromScanCode(0x1C, true))  /* shares VK_RETURN with Enter */      \
    /* Function keys (VK_F1..VK_F24) */                                                  \
    ENTRY(F1, fromVk(0x70))  ENTRY(F2, fromVk(0x71))  ENTRY(F3, fromVk(0x72))            \
    ENTRY(F4, fromVk(0x73))  ENTRY(F5, fromVk(0x74))  ENTRY(F6, fromVk(0x75))            \
    ENTRY(F7, fromVk(0x76))  ENTRY(F8, fromVk(0x77))  ENTRY(F9, fromVk(0x78))            \
    ENTRY(F10, fromVk(0x79)) ENTRY(F11, fromVk(0x7A)) ENTRY(F12, fromVk(0x7B))           \
    ENTRY(F13, fromVk(0x7C)) ENTRY(F14, fromVk(0x7D)) ENTRY(F15, fromVk(0x7E))           \
    ENTRY(F16, fromVk(0x7F)) ENTRY(F17, fromVk(0x80)) ENTRY(F18, fromVk(0x81))           \
    ENTRY(F19, fromVk(0x82)) ENTRY(F20, fromVk(0x83)) ENTRY(F21, fromVk(0x84))           \
    ENTRY(F22, fromVk(0x85)) ENTRY(F23, fromVk(0x86)) ENTRY(F24, fromVk(0x87))           \
    /* Modifiers */                                                                      \
    ENTRY(Shift, fromVk(0x10))   /* VK_SHIFT */                                          \
    ENTRY(LShift, fromVk(0xA0))  /* VK_LSHIFT */                                         \
    ENTRY(RShift, fromVk(0xA1))  /* VK_RSHIFT */                                         \
    ENTRY(Ctrl, fromVk(0x11))    /* VK_CONTROL */                                        \
    ENTRY(LCtrl, fromVk(0xA2))   /* VK_LCONTROL */                                       \
    ENTRY(RCtrl, fromVk(0xA3))   /* VK_RCONTROL */                                       \
    ENTRY(Alt, fromVk(0x12))     /* VK_MENU */                                           \
    ENTRY(LAlt, fromVk(0xA4))    /* VK_LMENU */                                          \
    ENTRY(RAlt, fromVk(0xA5))    /* VK_RMENU */                                          \
    ENTRY(AltGr, RAlt)                                                                   \
    ENTRY(LWin, fromVk(0x5B))    /* VK_LWIN */                                           \
    ENTRY(RWin, fromVk(0x5C))    /* VK_RWIN */                                           \
    ENTRY(Win, LWin)                                                                     \
    ENTRY(Apps, fromVk(0x5D))    /* VK_APPS, the context menu key */                     \
    /* Editing */                                                                        \
    ENTRY(Enter, fromVk(0x0D))        /* VK_RETURN */                                    \
    ENTRY(Esc, fromVk(0x1B))          /* VK_ESCAPE */                                    \
    ENTRY(Tab, fromVk(0x09))          /* VK_TAB */                                       \
    ENTRY(Space, fromVk(0x20))        /* VK_SPACE */                                     \
    ENTRY(Backspace, fromVk(0x08))    /* VK_BACK */                                      \
    ENTRY(CapsLock, fromVk(0x14))     /* VK_CAPITAL */                                   \
    ENTRY(NumLock, fromVk(0x90))      /* VK_NUMLOCK */                                   \
    ENTRY(ScrollLock, fromVk(0x91))   /* VK_SCROLL */                                    \
    ENTRY(PrintScreen, fromVk(0x2C))  /* VK_SNAPSHOT */                                  \
    ENTRY(Pause, fromVk(0x13))        /* VK_PAUSE */                                     \
    /* Navigation */                                                                     \
    ENTRY(Insert, fromVk(0x2D))    /* VK_INSERT */                                       \
    ENTRY(Delete, fromVk(0x2E))    /* VK_DELETE */                                       \
    ENTRY(Home, fromVk(0x24))      /* VK_HOME */                                         \
    ENTRY(End, fromVk(0x23))       /* VK_END */                                          \
    ENTRY(PageUp, fromVk(0x21))    /* VK_PRIOR */                                        \
    ENTRY(PageDown, fromVk(0x22))  /* VK_NEXT */                                         \
    ENTRY(Left, fromVk(0x25))      /* VK_LEFT */                                         \
    ENTRY(Up, fromVk(0x26))        /* VK_UP */                                           \
    ENTRY(Right, fromVk(0x27))     /* VK_RIGHT */                                        \
    ENTRY(Down, fromVk(0x28))      /* VK_DOWN */                                         \
    /* Punctuation keys whose meaning is the same in most layouts */                     \
    ENTRY(OemPlus, fromVk(0xBB))    /* VK_OEM_PLUS */                                    \
    ENTRY(OemComma, fromVk(0xBC))   /* VK_OEM_COMMA */                                   \
    ENTRY(OemMinus, fromVk(0xBD))   /* VK_OEM_MINUS */                                   \
    ENTRY(OemPeriod, fromVk(0xBE))  /* VK_OEM_PERIOD */                                  \
    /* Media */                                                                          \
    ENTRY(VolumeMute, fromVk(0xAD))      /* VK_VOLUME_MUTE */                            \
    ENTRY(VolumeDown, fromVk(0xAE))      /* VK_VOLUME_DOWN */                            \
    ENTRY(VolumeUp, fromVk(0xAF))        /* VK_VOLUME_UP */                              \
    ENTRY(MediaNext, fromVk(0xB0))       /* VK_MEDIA_NEXT_TRACK */                       \
    ENTRY(MediaPrev, fromVk(0xB1))       /* VK_MEDIA_PREV_TRACK */                       \
    ENTRY(MediaStop, fromVk(0xB2))       /* VK_MEDIA_STOP */                             \
    ENTRY(MediaPlayPause, fromVk(0xB3))  /* VK_MEDIA_PLAY_PAUSE */
// clang-format on
