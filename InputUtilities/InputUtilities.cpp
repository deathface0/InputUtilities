#include "InputUtilities.h"

bool InputUtilities::leftClick(time_t pressed_ms)
{
    bool success = MouseEvent(MOUSEEVENTF_LEFTDOWN);
    Sleep(pressed_ms);
    success &= MouseEvent(MOUSEEVENTF_LEFTUP);

    return success;
}

bool InputUtilities::rightClick(time_t pressed_ms)
{
    bool success = MouseEvent(MOUSEEVENTF_RIGHTDOWN);
    Sleep(pressed_ms);
    success &= MouseEvent(MOUSEEVENTF_RIGHTUP);

    return success;
}

bool InputUtilities::middleClick(time_t pressed_ms)
{
    bool success = MouseEvent(MOUSEEVENTF_MIDDLEDOWN);
    Sleep(pressed_ms);
    success &= MouseEvent(MOUSEEVENTF_MIDDLEUP);

    return success;
}

bool InputUtilities::extraClick(UINT button, time_t pressed_ms)
{
    bool success = ExtraClickDown(button);
    Sleep(pressed_ms);
    success &= ExtraClickUp(button);

    return success;
}

bool InputUtilities::vKey(WORD vkCode, time_t pressed_ms)
{
    bool success = vKeyDown(vkCode);
    Sleep(pressed_ms);
    success &= vKeyUp(vkCode);

    return success;
}

bool InputUtilities::unicodeKey(wchar_t key, time_t pressed_ms)
{
    bool success = unicodeKeyDown(key);
    Sleep(pressed_ms);
    success &= unicodeKeyUp(key);

    return success;
}

bool InputUtilities::scKey(wchar_t key, time_t pressed_ms)
{
    bool success = scKeyDown(key);
    Sleep(pressed_ms);
    success &= scKeyUp(key);

    return success;
}

bool InputUtilities::Key(Event e, time_t pressed_ms)
{
    bool success = keyDown(e);
    Sleep(pressed_ms);
    success &= keyUp(e);

    return success;
}

bool InputUtilities::vkMultiKey(const std::vector<WORD>& vkCodes, time_t pressed_ms)
{
    bool success = vkMultiKeyDown(vkCodes);
    Sleep(pressed_ms);
    success &= vkMultiKeyUp(vkCodes);

    return success;
}

bool InputUtilities::unicodeMultiKey(const std::vector<wchar_t>& keys, time_t pressed_ms)
{
    bool success = unicodeMultiKeyDown(keys);
    Sleep(pressed_ms);
    success &= unicodeMultiKeyUp(keys);

    return success;
}

bool InputUtilities::scMultiKey(const std::vector<wchar_t>& keys, time_t pressed_ms)
{
    bool success = scMultiKeyDown(keys);
    Sleep(pressed_ms);
    success &= scMultiKeyUp(keys);

    return success;
}

bool InputUtilities::typeStr(const std::wstring& str)
{
    bool success = true;

    for (wchar_t ch : str)
        success &= unicodeKey(ch);

    return success;
}

bool InputUtilities::scTypeStr(const std::wstring& str)
{
    bool success = true;

    for (wchar_t ch : str)
        success &= scKey(ch);

    return success;
}
