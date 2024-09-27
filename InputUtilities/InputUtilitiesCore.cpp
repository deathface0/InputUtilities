#include "InputUtilitiesCore.h"

InputUtilitiesCore::~InputUtilitiesCore()
{
    reset();
}

bool InputUtilitiesCore::SetCursorPos(int x, int y)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.time = 0;
    input.mi.dx = x * (65536 / GetSystemMetrics(SM_CXSCREEN));
    input.mi.dy = y * (65536 / GetSystemMetrics(SM_CYSCREEN));
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    return SendInput(1, &input, sizeof(INPUT));
}

bool InputUtilitiesCore::MouseEvent(DWORD m_event)
{
    if (isExtraMouseButton(m_event))
        return false;

    Event c_event{ m_event, 0xFF, 0xFF };

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = m_event;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        //this->runningInputs.push_back(c_event); //assert

        return success;
}

bool InputUtilitiesCore::ExtraClickDown(DWORD xbutton)
{
    Event c_event{ xbutton, 0xFF, 0xFF };

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = xbutton;
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

bool InputUtilitiesCore::ExtraClickUp(DWORD xbutton)
{
    Event c_event{ xbutton, 0xFF, 0xFF };

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = xbutton;
    input.mi.dwFlags = MOUSEEVENTF_XUP;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    try {
        bool success = SendInput(1, &input, sizeof(INPUT));

        if (success)
            removeEvent({ xbutton, (char)0xFF, (char)0xFF });

        return success;
    }
    catch (std::exception& e)
    {
        return false;
    }
}

bool InputUtilitiesCore::MouseWheelRoll(int scrolls, int delta)
{
    INPUT input = { 0 };
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
    bool success = true;
    bool upper = isUppercaseOn();

    if (!upper && isupper(vkCode) || (upper && islower(vkCode)))
    {
        Event c_event{ 0xFF, VK_SHIFT, (char)0xFF };

        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = 0;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            this->runningInputs.push_back(c_event);
    }

    char vk = static_cast<WORD>(toupper(vkCode));
    Event c_event{ 0xFF, vk, (char)0xFF };

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = 0;
    success &= SendInput(1, &input, sizeof(INPUT));

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
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        removeEvent({ 0xFF, vk, (char)0xFF });

    bool upper = isUppercaseOn();
    if (isupper(vkCode))
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            removeEvent({ 0xFF, VK_SHIFT, (char)0xFF });
    }

    return success;
}

bool InputUtilitiesCore::KeyDown(char key)
{
    bool success = true;
    bool upper = isUppercaseOn();

    if (!upper && isupper(key) || (upper && islower(key)))
    {
        Event c_event{ 0xFF, VK_SHIFT, 0xFF };

        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = 0;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            this->runningInputs.push_back(c_event);
    }

    char k = tolower(key);
    Event c_event{ 0xFF, 0xFF, k };

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan =
        static_cast<WORD>(MapVirtualKeyEx(VkKeyScanA(k), MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    success &= SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back(c_event);

    return success;
}

bool InputUtilitiesCore::KeyUp(char key)
{
    char k = tolower(key);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan =
        static_cast<WORD>(MapVirtualKeyEx(VkKeyScanA(k), MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        removeEvent({ 0xFF, (char)0xFF, k });

    bool upper = isUppercaseOn();
    if (isupper(key))
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            removeEvent({ 0xFF, VK_SHIFT, (char)0xFF });
    }

    return success;
}

bool InputUtilitiesCore::vkMultiKeyDown(const std::vector<char>& vkCodes)
{
    bool success = true;

    for (const auto& vkCode : vkCodes) {
        success &= this->vkKeyDown(vkCode);
        if (success)
            this->runningInputs.push_back({ 0xFF, vkCode, (char)0xFF });
    }

    return success;
}

bool InputUtilitiesCore::vkMultiKeyUp(const std::vector<char>& vkCodes)
{
    bool success = true;

    for (const auto& vkCode : vkCodes) {
        success &= this->vkKeyUp(vkCode);
        if (success)
            removeEvent({ 0xFF, vkCode, (char)0xFF });
    }

    return success;
}

bool InputUtilitiesCore::isUppercaseOn()
{
    bool isShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool isCapsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
    return isShiftPressed ^ isCapsLockOn;
}

bool InputUtilitiesCore::isExtraMouseButton(DWORD m_event)
{
    return (m_event != MOUSEEVENTF_LEFTDOWN &&
        m_event != MOUSEEVENTF_LEFTUP &&
        m_event != MOUSEEVENTF_RIGHTDOWN &&
        m_event != MOUSEEVENTF_RIGHTUP &&
        m_event != MOUSEEVENTF_MIDDLEDOWN &&
        m_event != MOUSEEVENTF_MIDDLEUP)
        ? true : false;
}

bool InputUtilitiesCore::removeEvent(const Event& e)
{
    auto originalSize = runningInputs.size();

    this->runningInputs.erase(
        std::remove_if(runningInputs.begin(), runningInputs.end(),
            [&e](const Event& ev) {
                return ev.mouse == e.mouse && ev.vk == e.vk && ev.key == e.key;
            }),
        runningInputs.end()
    );

    return runningInputs.size() < originalSize;
}

void InputUtilitiesCore::reset()
{
    for (const auto& input : runningInputs)
    {
        if (input.vk != 0xFF)
            vkKeyUp(input.vk);

        if (input.key != 0xFF)
            KeyUp(input.key);

        if (input.mouse != 0xFF)
            MouseEvent(input.mouse << 1); //Bit shift left to obtain UP equivalence
    }
}