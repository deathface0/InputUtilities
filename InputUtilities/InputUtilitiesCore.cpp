#include "InputUtilitiesCore.h"

InputUtilitiesCore::InputUtilitiesCore(bool safemode)
    :safemode(safemode)
{
}

InputUtilitiesCore::~InputUtilitiesCore()
{
    if (safemode)
        reset();
}

Result InputUtilitiesCore::SetCursorPos(int x, int y, bool abs)
{
    POINT p1, p2;
    if (safemode)
        GetCursorPos(&p1);
    
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.time = 0;
    input.mi.dx = abs ? (x * (65536 / GetSystemMetrics(SM_CXSCREEN))) : x;
    input.mi.dy = abs ? (y * (65536 / GetSystemMetrics(SM_CYSCREEN))) : y;
    input.mi.dwFlags = abs ? MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE : MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));

    if (safemode) {
        GetCursorPos(&p2);
        bool success = (p1.x != p2.x && p1.y != p2.y) ? true : false;
        return !success ? MOUSE_SAME_POSITON : SUCCESS;
    }
    
    return Result();
}

Result InputUtilitiesCore::SetCursorPos(int x, int y, int steps, int delay, bool abs)
{
    POINT p1, p2;
    GetCursorPos(&p1);

    int startX = p1.x;
    int startY = p1.y;
    int endX = x;
    int endY = y;

    for (int i = 1; i <= steps; ++i)
    {
        double t = static_cast<double>(i) / steps; // Progress from 0.0 to 1.0

        // Smooth interpolation using ease-in-out formula
        double smoothT = t * t * (3 - 2 * t);

        int newX = startX + static_cast<int>((endX - startX) * smoothT);
        int newY = startY + static_cast<int>((endY - startY) * smoothT);

        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.time = 0;
        input.mi.dx = abs ? (newX * (65536 / GetSystemMetrics(SM_CXSCREEN))) : newX;
        input.mi.dy = abs ? (newY * (65536 / GetSystemMetrics(SM_CYSCREEN))) : newY;
        input.mi.dwFlags = abs ? MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE : MOUSEEVENTF_MOVE;

        SendInput(1, &input, sizeof(INPUT));

        Sleep(delay);
    }

    GetCursorPos(&p2);
    bool success = (p1.x != p2.x && p1.y != p2.y);
    return !success ? MOUSE_SAME_POSITON : SUCCESS;
}


Result InputUtilitiesCore::MouseEvent(WORD m_event)
{
    if (isExtraMouseButton(m_event))
        return false;

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = m_event;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (!safemode || !success)
        return Result();

    if (isButtonUp(m_event))
        this->runningInputs.erase({ InputType::MOUSE, m_event });
    else
        this->runningInputs.insert({ InputType::MOUSE, m_event });

    return Result();
}

Result InputUtilitiesCore::ExtraClickDown(WORD xbutton)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.mouseData = xbutton;
    input.mi.dwFlags = MOUSEEVENTF_XDOWN;

    bool success = SendInput(1, &input, sizeof(INPUT));
    if (success && safemode)
        this->runningInputs.insert({ InputType::MOUSE, xbutton });

    return Result();
}

Result InputUtilitiesCore::ExtraClickUp(WORD xbutton)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.mouseData = xbutton;
    input.mi.dwFlags = MOUSEEVENTF_XUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success && safemode)
        this->runningInputs.erase({ InputType::MOUSE, xbutton });

    return Result();
}

Result InputUtilitiesCore::MouseWheelRoll(int scrolls, MWheelDir dir, UINT delta, MWheelAxis axis)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.mouseData = scrolls * dir * delta;
    input.mi.dwFlags = axis;
    SendInput(1, &input, sizeof(INPUT));

    return Result();
}

Result InputUtilitiesCore::MouseWheelRoll(int scrolls, int delay, MWheelDir dir, UINT delta, MWheelAxis axis)
{
    for (int i = 0; i < scrolls; i++) {
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.mouseData = dir * delta;
        input.mi.dwFlags = axis;
        SendInput(1, &input, sizeof(INPUT));

        Sleep(delay);
    }

    return Result();
}

Result InputUtilitiesCore::vKeyDown(WORD vkCode)
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = 0;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success && safemode)
        this->runningInputs.insert({ InputType::VK, vkCode });

    return Result();
}

Result InputUtilitiesCore::vKeyUp(WORD vkCode)
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success && safemode)
        this->runningInputs.erase({ InputType::VK, vkCode });

    return Result();
}

Result InputUtilitiesCore::unicodeKeyDown(wchar_t key)
{
    WORD wk = static_cast<WORD>(key);
    
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan = static_cast<WORD>(key);
    input.ki.dwFlags = KEYEVENTF_UNICODE;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success && safemode)
        this->runningInputs.insert({ InputType::UC, wk });

    return Result();
}

Result InputUtilitiesCore::unicodeKeyUp(wchar_t key)
{
    WORD wk = static_cast<WORD>(key);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan = wk;
    input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success && safemode)
        this->runningInputs.erase({ InputType::UC, wk });

    return Result();
}

Result InputUtilitiesCore::scKeyDown(wchar_t key)
{
    WORD vk = LOBYTE(VkKeyScanW(key));
    if (vk == 0xFF)
        return false;

    WORD scancode = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, GetKeyboardLayout(0));

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scancode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    bool success = (SendInput(1, &input, sizeof(INPUT)) == 1);

    if (success && safemode)
        this->runningInputs.insert({ InputType::SC, scancode });

    return Result();
}


Result InputUtilitiesCore::scKeyUp(wchar_t key)
{
    BYTE vk = VkKeyScanW(key);
    if (vk == 0xFF)
        return false;

    WORD scancode = MapVirtualKeyEx(vk & 0xFF, MAPVK_VK_TO_VSC, GetKeyboardLayout(0));

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scancode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success && safemode)
        this->runningInputs.erase({ InputType::SC, scancode });

    return Result();
}

Result InputUtilitiesCore::keyDown(Event e)
{
    Result result;

    switch (e.type) {
    case InputType::VK:
        result = this->vKeyDown(e.iu_event.vkKey);
        break;
    case InputType::UC:
        result = this->unicodeKeyDown(e.iu_event.charKey);
        break;
    case InputType::SC:
        result = this->scKeyDown(e.iu_event.charKey);
        break;
    default:
        result.errorcode = UNDEFINED_INPUT_TYPE;
    }

    return result;
}

Result InputUtilitiesCore::keyUp(Event e)
{
    Result result;

    switch (e.type) {
    case InputType::VK:
        result = this->vKeyUp(e.iu_event.vkKey);
        break;
    case InputType::UC:
        result = this->unicodeKeyUp(e.iu_event.charKey);
        break;
    case InputType::SC:
        result = this->scKeyUp(e.iu_event.charKey);
        break;
    default:
        result.errorcode = UNDEFINED_INPUT_TYPE;
    }

    return Result();
}

Result InputUtilitiesCore::vkMultiKeyDown(const std::vector<WORD>& vkCodes)
{
    Result result;

    for (const auto& vk : vkCodes) {
        result = this->vKeyDown(vk);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

Result InputUtilitiesCore::vkMultiKeyUp(const std::vector<WORD>& vkCodes)
{
    Result result;

    for (const auto& vk : vkCodes) {
        result = this->vKeyUp(vk);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

Result InputUtilitiesCore::unicodeMultiKeyDown(const std::vector<wchar_t>& keys)
{
    Result result;

    for (const auto& key : keys) {
        result = this->unicodeKeyDown(key);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

Result InputUtilitiesCore::unicodeMultiKeyUp(const std::vector<wchar_t>& keys)
{
    Result result;

    for (const auto& key : keys) {
        result = this->unicodeKeyUp(key);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

Result InputUtilitiesCore::scMultiKeyDown(const std::vector<Key>& keys)
{
    Result result;

    for (const auto& key : keys) {
        result = key.isVK ? this->vKeyDown(key.vkKey) : this->scKeyDown(key.charKey);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

Result InputUtilitiesCore::scMultiKeyUp(const std::vector<Key>& keys)
{
    Result result;

    for (const auto& key : keys) {
        result = key.isVK ? this->vKeyUp(key.vkKey) : this->scKeyUp(key.charKey);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

Result InputUtilitiesCore::multiKeyDown(const std::vector<Event>& keys)
{
    Result result;

    for (const auto& key : keys) {
        result = this->keyDown(key);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

Result InputUtilitiesCore::multiKeyUp(const std::vector<Event>& keys)
{
    Result result;

    for (const auto& key : keys) {
        result = this->keyUp(key);
        if (result.errorcode != 0x0)
            break;
    }

    return result;
}

std::string InputUtilitiesCore::get_utf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int sz = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), 0, 0, 0, 0);
    std::string res(sz, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &res[0], sz, 0, 0);
    return res;
}

std::wstring InputUtilitiesCore::get_utf16(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int sz = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), 0, 0);
    std::wstring res(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &res[0], sz);
    return res;
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

void InputUtilitiesCore::reset()
{
    for (const auto& input : runningInputs)
    {
        switch (input.type)
        {
        case InputType::VK:
            vKeyUp(input.iu_event.vkKey);
            break;
        case InputType::UC:
            unicodeKeyUp(input.iu_event.charKey);
            break;
        case InputType::SC:
            scKeyUp(input.iu_event.charKey);
            break;
        case InputType::MOUSE:
            MouseEvent(input.iu_event.vkKey << 1); //Bit shift left to obtain UP equivalence
            break;
        }
    }
}