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

bool InputUtilities::vkKey(DWORD vkCode, time_t pressed_ms)
{
    bool success = vkKeyDown(vkCode);
    Sleep(pressed_ms);
    success &= vkKeyUp(vkCode);

    return success;
}

bool InputUtilities::mappedKey(char key, time_t pressed_ms)
{
    bool success = mappedKeyDown(key);
    Sleep(pressed_ms);
    success &= mappedKeyUp(key);

    return success;
}

bool InputUtilities::Key(DWORD vkCode, time_t pressed_ms)
{
    bool success = keyDown(vkCode);
    Sleep(pressed_ms);
    success &= keyUp(vkCode);

    return success;
}

bool InputUtilities::vkMultiKey(const std::vector<DWORD>& vkCodes, time_t pressed_ms)
{
    bool success = vkMultiKeyDown(vkCodes);
    Sleep(pressed_ms);
    success &= vkMultiKeyUp(vkCodes);

    return success;
}

bool InputUtilities::mappedMultiKey(const std::vector<char>& keys, time_t pressed_ms)
{
    bool success = mappedMultiKeyDown(keys);
    Sleep(pressed_ms);
    success &= mappedMultiKeyUp(keys);

    return success;
}

bool InputUtilities::MultiKey(const std::vector<DWORD>& vkCodes, time_t pressed_ms)
{
    bool success = MultiKeyDown(vkCodes);
    Sleep(pressed_ms);
    success &= MultiKeyUp(vkCodes);

    return success;
}

bool InputUtilities::typeStr(const std::string& str)
{
    bool success = true;

    for (char ch : str)
        success &= Key(static_cast<DWORD>(ch));

    return success;
}
