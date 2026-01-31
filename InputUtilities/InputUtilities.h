#pragma once

#include "InputUtilitiesCore.h"
#include <string>

class InputUtilities : public InputUtilitiesCore
{
public:
    InputUtilities(bool safemode = false)
        : InputUtilitiesCore(safemode) {
    };

    InputResult leftClick(int pressed_ms = 0);
    InputResult rightClick(int pressed_ms = 0);
    InputResult middleClick(int pressed_ms = 0);
    InputResult extraClick(UINT button, int pressed_ms = 0);

    InputResult vKey(WORD vkCode, int pressed_ms = 0);
    InputResult unicodeKey(wchar_t key, int pressed_ms = 0);
    InputResult scKey(wchar_t key, int pressed_ms = 0);
    InputResult Key(Event e, int pressed_ms = 0);

    InputResult vkMultiKey(const std::vector<WORD>& vkCodes, int pressed_ms = 0);
    InputResult unicodeMultiKey(const std::vector<wchar_t>& keys, int pressed_ms = 0);
    InputResult scMultiKey(const std::vector<wchar_t>& keys, int pressed_ms = 0);
    InputResult multiKey(const std::vector<Event>& keys, int pressed_ms = 0);

    InputResult typeStr(const std::wstring& str, int char_delay = 0);
    InputResult scTypeStr(const std::wstring& str, int char_delay = 0);
};