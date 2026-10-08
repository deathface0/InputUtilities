#include "inpututil/InputUtilities.h"

#define PRESS_RELEASE_LOGIC(downCall, upCall) \
    InputResult r1 = downCall; \
    if (pressed_ms > 0) Sleep(pressed_ms); \
    InputResult r2 = upCall; \
    return (r1 != InputResult::Success) ? r1 : r2;

InputResult InputUtilities::leftClick(int pressed_ms)
{
    PRESS_RELEASE_LOGIC(MouseEvent(MOUSEEVENTF_LEFTDOWN), MouseEvent(MOUSEEVENTF_LEFTUP));
}

InputResult InputUtilities::rightClick(int pressed_ms)
{
    PRESS_RELEASE_LOGIC(MouseEvent(MOUSEEVENTF_RIGHTDOWN), MouseEvent(MOUSEEVENTF_RIGHTUP));
}

InputResult InputUtilities::middleClick(int pressed_ms)
{
    PRESS_RELEASE_LOGIC(MouseEvent(MOUSEEVENTF_MIDDLEDOWN), MouseEvent(MOUSEEVENTF_MIDDLEUP));
}

InputResult InputUtilities::extraClick(UINT button, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(MouseEvent(MOUSEEVENTF_XDOWN, button), MouseEvent(MOUSEEVENTF_XUP, button));
}

InputResult InputUtilities::vKey(WORD vkCode, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(vKeyDown(vkCode), vKeyUp(vkCode));
}

InputResult InputUtilities::unicodeKey(wchar_t key, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(unicodeKeyDown(key), unicodeKeyUp(key));
}

InputResult InputUtilities::scKey(wchar_t key, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(scKeyDown(key), scKeyUp(key));
}

InputResult InputUtilities::Key(Event e, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(keyDown(e), keyUp(e));
}

InputResult InputUtilities::vkMultiKey(const std::vector<WORD>& vkCodes, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(vkMultiKeyDown(vkCodes), vkMultiKeyUp(vkCodes));
}

InputResult InputUtilities::unicodeMultiKey(const std::vector<wchar_t>& keys, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(unicodeMultiKeyDown(keys), unicodeMultiKeyUp(keys));
}

InputResult InputUtilities::scMultiKey(const std::vector<wchar_t>& keys, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(scMultiKeyDown(keys), scMultiKeyUp(keys));
}

InputResult InputUtilities::multiKey(const std::vector<Event>& keys, int pressed_ms)
{
    PRESS_RELEASE_LOGIC(multiKeyDown(keys), multiKeyUp(keys));
}

InputResult InputUtilities::typeStr(const std::wstring& str, int char_delay)
{
    InputResult res = Success;
    for (size_t i = 0; i < str.length(); ++i) {
        res = unicodeKey(str[i]);
        if (res != Success) break;

        if (char_delay > 0 && i < str.length() - 1) Sleep(char_delay);
    }
    return res;
}

InputResult InputUtilities::scTypeStr(const std::wstring& str, int char_delay)
{
    InputResult res = Success;
    for (size_t i = 0; i < str.length(); ++i) {
        res = scKey(str[i]);
        if (res != Success) break;

        if (char_delay > 0 && i < str.length() - 1) Sleep(char_delay);
    }
    return res;
}

#undef PRESS_RELEASE_LOGIC