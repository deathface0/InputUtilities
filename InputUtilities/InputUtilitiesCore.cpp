#include "InputUtilitiesCore.h"

std::ostream& operator<<(std::ostream& os, const Event& e) {
    os << "{m: " << e.mouse << ", vk: " << (DWORD)(char)e.vk << ", key: " << (DWORD)(char)e.key << "}" << std::endl;
    return os;
}

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


    Event c_event(m_event, 0, 0);

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = m_event;
    bool success = SendInput(1, &input, sizeof(INPUT));


    if (!success) 
        return false;

    if (isButtonUp(m_event))
        removeEvent(Event(c_event.mouse >> 1, 0, 0));
    else
        this->runningInputs.push_back(c_event);

    return success;
}

bool InputUtilitiesCore::ExtraClickDown(DWORD xbutton)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = xbutton;
    input.mi.dwFlags = MOUSEEVENTF_XDOWN;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    bool success = SendInput(1, &input, sizeof(INPUT));
    if (success)
        this->runningInputs.push_back(Event(xbutton, 0, 0));

    return success;
}

bool InputUtilitiesCore::ExtraClickUp(DWORD xbutton)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.mouseData = xbutton;
    input.mi.dwFlags = MOUSEEVENTF_XUP;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    bool success = SendInput(1, &input, sizeof(INPUT));
    if (success)
        removeEvent(Event(xbutton, 0, 0));

    return success;
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

bool InputUtilitiesCore::vkKeyDown(DWORD vkCode)
{
    bool success = true;

    BYTE modifiers = HIBYTE(vkCode);

    bool upper = isUppercaseOn();
    bool isVkeyCode = isVKey(vkCode);
    char c_vkey = isVkeyCode ? 0 : static_cast<char>(vkCode);
    bool isKeyUpper = c_vkey != 0 && ((!upper && isupper(c_vkey)) || (upper && islower(c_vkey)));

    if (isKeyUpper || modifiers & 1)
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = 0;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            this->runningInputs.push_back(Event(0, VK_SHIFT, 0));
    }

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = isVkeyCode ? vkCode : toupper(c_vkey);
    input.ki.dwFlags = 0;
    success &= SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back(Event(0, vkCode, 0));

    return success;
}

bool InputUtilitiesCore::vkKeyUp(DWORD vkCode)
{
    bool isVkeyCode = isVKey(vkCode);
    char c_vkey = isVkeyCode ? 0 : static_cast<char>(vkCode);
    BYTE modifiers = HIBYTE(vkCode);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = isVkeyCode ? vkCode : toupper(c_vkey);
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        removeEvent(Event(0, vkCode, 0));

    if (isupper(c_vkey) || modifiers & 1)
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            removeEvent(Event(0, VK_SHIFT, 0));
    }

    return success;
}

bool InputUtilitiesCore::mappedKeyDown(char key)
{
    if (isVKey(key))
        return false;

    bool success = true;
    bool upper = isUppercaseOn();

    if ((!upper && isupper(key)) || (upper && islower(key)))
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = 0;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            this->runningInputs.push_back(Event(0, VK_SHIFT, 0));
    }

    char k = tolower(key);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan =
        static_cast<WORD>(MapVirtualKeyEx(VkKeyScanA(k), MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    success &= SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back(Event(0, 0, k));

    return success;
}

bool InputUtilitiesCore::mappedKeyUp(char key)
{
    if (isVKey(key))
        return false;

    char k = tolower(key);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan =
        static_cast<WORD>(MapVirtualKeyEx(VkKeyScanA(k), MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        removeEvent(Event(0, 0, k));

    if (isupper(key))
    {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_SHIFT;
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        success &= SendInput(1, &input, sizeof(INPUT));

        if (success)
            removeEvent(Event(0, VK_SHIFT, 0));
    }

    return success;
}

bool InputUtilitiesCore::keyDown(DWORD vkCode)
{
    bool success = true;

    if (isVKey(vkCode))
    {
        success &= this->vkKeyDown(vkCode);
    }
    else {
        char c_key = static_cast<char>(vkCode);
        success &= this->mappedKeyDown(c_key);
    }

    return success;
}

bool InputUtilitiesCore::keyUp(DWORD vkCode)
{
    bool success = true;

    if (isVKey(vkCode))
    {
        success &= this->vkKeyUp(vkCode);
    }
    else {
        char c_key = static_cast<char>(vkCode);
        success &= this->mappedKeyUp(c_key);
    }

    return success;
}

bool InputUtilitiesCore::vkMultiKeyDown(const std::vector<DWORD>& vkCodes)
{
    bool success = true;

    for (const auto& vkCode : vkCodes) {
        success &= this->vkKeyDown(vkCode);
        if (success)
            this->runningInputs.push_back(Event(0, vkCode, 0));
        else
            break;
    }

    return success;
}

bool InputUtilitiesCore::vkMultiKeyUp(const std::vector<DWORD>& vkCodes)
{
    bool success = true;

    for (const auto& vkCode : vkCodes) {
        success &= this->vkKeyUp(vkCode);
        if (success)
            removeEvent(Event(0, vkCode, 0));
    }

    return success;
}

bool InputUtilitiesCore::mappedMultiKeyDown(const std::vector<char>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        success &= this->mappedKeyDown(key);
        if (success)
            this->runningInputs.push_back(Event(0, 0, key));
        else
            break;
    }

    return success;
}

bool InputUtilitiesCore::mappedMultiKeyUp(const std::vector<char>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        success &= this->mappedKeyUp(key);
        if (success)
            removeEvent(Event(0, 0, key));
    }

    return success;
}

bool InputUtilitiesCore::MultiKeyDown(const std::vector<DWORD>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        if (isVKey(key))
        {
            success &= this->vkKeyDown(key);
            if (success)
                this->runningInputs.push_back(Event(0, key, 0));
        }
        else {
            char c_key = static_cast<char>(key);
            success &= this->mappedKeyDown(c_key);
            if (success)
                this->runningInputs.push_back(Event(0, 0, c_key));
        }
    }

    return success;
}

bool InputUtilitiesCore::MultiKeyUp(const std::vector<DWORD>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        char c_key = static_cast<char>(key);

        success &= isVKey(key)
            ? (this->vkKeyUp(c_key) ? removeEvent(Event(0, c_key, 0)) : false)
            : (this->mappedKeyUp(c_key) ? removeEvent(Event(0, 0, c_key)) : false);
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

bool InputUtilitiesCore::isButtonUp(DWORD button)
{
    return (button == MOUSEEVENTF_LEFTUP || button == MOUSEEVENTF_RIGHTUP || button == MOUSEEVENTF_MIDDLEUP) ? true : false;
}

bool InputUtilitiesCore::isVKey(DWORD key)
{
    char c_key = static_cast<char>(key);

    return (c_key >= 65 && c_key <= 93  ||
            c_key >= 96 && c_key <= 122 || 
            c_key >= 43 && c_key <= 57  || 
            c_key == 39 || c_key == 59  || c_key == 61)
            ? false : true;
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
        if (input.vk != 0)
            vkKeyUp(input.vk);

        if (input.key != 0)
            mappedKeyUp(input.key);

        if (input.mouse != 0)
            MouseEvent(input.mouse << 1); //Bit shift left to obtain UP equivalence
    }
    this->runningInputs.clear();
}