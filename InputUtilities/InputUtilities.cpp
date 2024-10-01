#include "InputUtilities.h"

bool InputUtilities::extraClick(int button)
{
    INPUT input = { 0 };
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
        return false;
    }
}

bool InputUtilities::vkKey(WORD vkCode)
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

bool InputUtilities::directKey(char key)
{
    int flag = 0;

    INPUT input = { 0 };
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

bool InputUtilities::vkMultiKey(const std::vector<WORD>& vkCodes)
{
    int flag = 0;
    INPUT input = { 0 };

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