#include "InputUtilities.h"

InputUtilitiesCore::~InputUtilitiesCore()
{
    reset();
}

bool InputUtilitiesCore::SetCursorPos(int x, int y)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.time = 0;
    input.mi.dx = x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    input.mi.dy = y * (65536 / GetSystemMetrics(SM_CYSCREEN));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    return SendInput(1, &input, sizeof(INPUT));
}

bool InputUtilitiesCore::MouseEvent(WORD m_event)
{ 
    if (m_event != MOUSEEVENTF_LEFTDOWN || m_event != MOUSEEVENTF_RIGHTDOWN && m_event != MOUSEEVENTF_MIDDLEDOWN)
        return false;
    
    Event c_event{ m_event, -1, -1 };

    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = m_event;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back(c_event);

    return success;
}

bool InputUtilitiesCore::ExtraClickDown(int button)
{
    Event c_event{ MOUSEEVENTF_XDOWN, -1, -1 };

    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = XBUTTON1 + (button - 1); // Se calcula el número de botón extra en función del parámetro
    input.mi.dwFlags = MOUSEEVENTF_XDOWN;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    try {
        this->runningInputs.push_back(c_event);
        return SendInput(1, &input, sizeof(INPUT));
    }
    catch (std::exception& e)
    {
        return false;
    }
}

bool InputUtilitiesCore::ExtraClickUp(int button)
{
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = XBUTTON1 + (button - 1); // Se calcula el número de botón extra en función del parámetro
    input.mi.dwFlags = MOUSEEVENTF_XUP;
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

bool InputUtilitiesCore::MouseWheelRoll(int scrolls, int delta)
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

bool InputUtilitiesCore::vkKeyDown(char vkCode)
{
    char vk = static_cast<WORD>(toupper(vkCode));
    Event c_event{ -1, vk, -1 };

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = 0;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back(c_event);

    return success;
}

bool InputUtilitiesCore::vkKeyUp(char vkCode)
{
    char vk = static_cast<WORD>(toupper(vkCode));

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(1, &input, sizeof(INPUT));
}

bool InputUtilitiesCore::KeyDown(char key)
{
    char k = tolower(key);
    Event c_event{ -1, -1, k };

    INPUT input;
    memset(&input, 0, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan =
        static_cast<WORD>(MapVirtualKeyEx(VkKeyScanA(k), MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back(c_event);

    return success;
}

bool InputUtilitiesCore::KeyUp(char key)
{
    char k = tolower(key);

    INPUT input;
    memset(&input, 0, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan =
        static_cast<WORD>(MapVirtualKeyEx(VkKeyScanA(k), MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    return SendInput(1, &input, sizeof(INPUT));
}

bool InputUtilitiesCore::vkMultiKeyDown(const std::vector<char>& vkCodes)
{
    int flag = 0;
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    for (const auto& vkCode : vkCodes) {
        char vk = static_cast<WORD>(toupper(vkCode));
        Event c_event{ -1, vk, -1 };

        input.ki.wVk = vk;
        input.ki.dwFlags = 0;
        bool success = SendInput(1, &input, sizeof(INPUT));

        if (success)
        {
            this->runningInputs.push_back(c_event);
            flag++;
        }
    }

    return (flag == vkCodes.size()) ? true : false;
}

bool InputUtilitiesCore::vkMultiKeyUp(const std::vector<char>& vkCodes)
{
    int flag = 0;
    INPUT input;

    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    for (const auto& vkCode : vkCodes) {
        char vk = static_cast<WORD>(toupper(vkCode));

        input.ki.wVk = vk;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        flag += SendInput(1, &input, sizeof(INPUT));
    }

    return (flag == vkCodes.size()) ? true : false;
}

void InputUtilitiesCore::reset()
{
    for (const auto& input : runningInputs)
    {
        if (input.vk != -1) 
            vkKeyUp(input.vk);

        if (input.key != -1)
            KeyUp(input.key);

        if (input.mouse != -1)
            MouseEvent(input.mouse << 1); //Bit shift left to obtain UP equivalence
    }
}










bool InputUtilities::extraClick(int button)
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

bool InputUtilities::vkMultiKey(const std::vector<WORD>& vkCodes)
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
