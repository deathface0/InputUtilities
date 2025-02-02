#include "InputUtilities.h"

Result InputUtilities::leftClick(time_t pressed_ms)
{
    Result result = MouseEvent(LEFTDOWN);
    Sleep(pressed_ms);
    MouseEvent(MOUSEEVENTF_LEFTUP);

    return result;
}

Result InputUtilities::rightClick(time_t pressed_ms)
{
    Result result = MouseEvent(RIGHTDOWN);
    Sleep(pressed_ms);
    MouseEvent(MOUSEEVENTF_RIGHTUP);

    return result;
}

Result InputUtilities::middleClick(time_t pressed_ms)
{
    Result result = MouseEvent(MIDDLEDOWN);
    Sleep(pressed_ms);
    MouseEvent(MOUSEEVENTF_MIDDLEUP);

    return result;
}

Result InputUtilities::extraClick(UINT button, time_t pressed_ms)
{
    Result result = ExtraClickDown(button);
    Sleep(pressed_ms);
    ExtraClickUp(button);

    return result;
}

Result InputUtilities::vKey(WORD vkCode, time_t pressed_ms)
{
    Result result = vKeyDown(vkCode);
    Sleep(pressed_ms);
    vKeyUp(vkCode);

    return result;
}

Result InputUtilities::unicodeKey(wchar_t key, time_t pressed_ms)
{
    Result result = unicodeKeyDown(key);
    Sleep(pressed_ms);
    unicodeKeyUp(key);

    return result;
}

Result InputUtilities::scKey(wchar_t key, time_t pressed_ms)
{
    Result result = scKeyDown(key);
    Sleep(pressed_ms);
    scKeyUp(key);

    return result;
}

Result InputUtilities::Key(Event e, time_t pressed_ms)
{
    Result result = keyDown(e);
    Sleep(pressed_ms);
    keyUp(e);

    return result;
}

Result InputUtilities::vkMultiKey(const std::vector<WORD>& vkCodes, time_t pressed_ms)
{
    Result result = vkMultiKeyDown(vkCodes);
    Sleep(pressed_ms);
    vkMultiKeyUp(vkCodes);

    return result;
}

Result InputUtilities::unicodeMultiKey(const std::vector<wchar_t>& keys, time_t pressed_ms)
{
    Result result = unicodeMultiKeyDown(keys);
    Sleep(pressed_ms);
    unicodeMultiKeyUp(keys);

    return result;
}

Result InputUtilities::scMultiKey(const std::vector<struct Key>& keys, time_t pressed_ms)
{
    Result result = scMultiKeyDown(keys);
    Sleep(pressed_ms);
    scMultiKeyUp(keys);

    return result;
}

Result InputUtilities::typeStr(const std::wstring& str)
{
    Result result;

    for (wchar_t ch : str)
        result.errorcode &= unicodeKey(ch).errorcode;

    return result;
}

Result InputUtilities::scTypeStr(const std::wstring& str)
{
    Result result;

    for (wchar_t ch : str)
        result.errorcode &= scKey(ch).errorcode;

    return result;
}
