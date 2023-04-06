#include "InputUtilities.h"

bool MouseUtilities::move(int x, int y)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    input.mi.dy = y * (65536 / GetSystemMetrics(SM_CYSCREEN));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    return SendInput(1, &input, sizeof(INPUT));
}

bool MouseUtilities::rightClick()
{
    int flag = 0;

    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    flag += SendInput(1, &input, sizeof(INPUT));
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    flag += SendInput(1, &input, sizeof(INPUT));

    return (flag == 2) ? true : false;
}

bool MouseUtilities::leftClick()
{
    int flag = 0;

    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    flag += SendInput(1, &input, sizeof(INPUT));
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    flag += SendInput(1, &input, sizeof(INPUT));

    return (flag == 2) ? true : false;
}

bool MouseUtilities::wheelClick()
{
    int flag = 0;

    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
    flag += SendInput(1, &input, sizeof(INPUT));
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
    flag += SendInput(1, &input, sizeof(INPUT));

    return (flag == 2) ? true : false;
}

bool MouseUtilities::extraClick(int button)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = XBUTTON1 + (button - 1); // Se calcula el número de botón extra en función del parámetro
    input.mi.dwFlags = MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    try {
        return SendInput(1, &input, sizeof(INPUT));
    }
    catch (std::exception& e)
    {
    }
}

bool MouseUtilities::wheelRoll(int scrolls, int delta)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = scrolls * delta * WHEEL_DELTA;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;
    return SendInput(1, &input, sizeof(INPUT));
}

















bool KeyboardUtilities::virtualInput(WORD vkCode)
{
    int flag = 0;

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = 0;
    flag += SendInput(1, &input, sizeof(INPUT));
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    flag += SendInput(1, &input, sizeof(INPUT));

    return (flag == 2) ? true : false;
}

bool KeyboardUtilities::directInput(char key)
{
    int flag = 0;

    INPUT input;
    memset(&input, 0, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan =
        static_cast<WORD>(MapVirtualKeyEx(VkKeyScanA(key), MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    flag += SendInput(1, &input, sizeof(INPUT));

    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    flag += SendInput(1, &input, sizeof(INPUT));

    return (flag == 2) ? true : false;
}

bool KeyboardUtilities::virtualCombinedInput(const std::vector<WORD>& vkCodes)
{
    int flag = 0;
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    for (const auto& vkCode : vkCodes) {
        input.ki.wVk = vkCode;
        input.ki.dwFlags = 0;
        flag += SendInput(1, &input, sizeof(INPUT));
    }

    for (const auto& vkCode : vkCodes) {
        input.ki.wVk = vkCode;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        flag += SendInput(1, &input, sizeof(INPUT));
    }

    return (flag == vkCodes.size() * 2) ? true : false;
}

void KeyboardUtilities::typeStr(const char* str)
{
    char cChar;
    while ((cChar = *str++)) // loops through chars
    {
        short vk = VkKeyScan(cChar); // keycode of char
        if ((vk >> 8) & 1) { keybd_event(VK_LSHIFT, 0, 0, 0); } // hold shift if necessary
        keybd_event((unsigned char)vk, 0, 0, 0); // key in
        keybd_event((unsigned char)vk, 0, KEYEVENTF_KEYUP, 0); // key out
        if ((vk >> 8) & 1) { keybd_event(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0); } // release shift if necessary
    }
}

void KeyboardUtilities::typeStr(char* str)
{
    char cChar;
    while ((cChar = *str++)) // loops through chars
    {
        short vk = VkKeyScan(cChar); // keycode of char
        if ((vk >> 8) & 1) { keybd_event(VK_LSHIFT, 0, 0, 0); } // hold shift if necessary
        keybd_event((unsigned char)vk, 0, 0, 0); // key in
        keybd_event((unsigned char)vk, 0, KEYEVENTF_KEYUP, 0); // key out
        if ((vk >> 8) & 1) { keybd_event(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0); } // release shift if necessary
    }
}