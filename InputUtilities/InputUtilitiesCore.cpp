﻿#include "InputUtilitiesCore.h"

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

bool InputUtilitiesCore::MouseEvent(WORD m_event)
{
    if (isExtraMouseButton(m_event))
        return false;

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = m_event;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (!success) 
        return false;

    if (isButtonUp(m_event))
        removeEvent({ IU_TYPE::IU_MOUSE, m_event });
    else
        this->runningInputs.push_back({ IU_TYPE::IU_MOUSE, m_event });

    return success;
}

bool InputUtilitiesCore::ExtraClickDown(WORD xbutton)
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
        this->runningInputs.push_back({ IU_TYPE::IU_MOUSE, xbutton });

    return success;
}

bool InputUtilitiesCore::ExtraClickUp(WORD xbutton)
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
        removeEvent({ IU_TYPE::IU_MOUSE, xbutton });

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

bool InputUtilitiesCore::vKeyDown(WORD vkCode)
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = 0;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back({ IU_TYPE::IU_VK, vkCode });

    return success;
}

bool InputUtilitiesCore::vKeyUp(WORD vkCode)
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        removeEvent(Event(IU_TYPE::IU_VK, vkCode));

    return success;
}

bool InputUtilitiesCore::unicodeKeyDown(wchar_t key)
{
    WORD wk = static_cast<WORD>(key);
     
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan = wk;
    input.ki.dwFlags = KEYEVENTF_UNICODE;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back({ IU_TYPE::IU_UC, wk });

    return success;
}

bool InputUtilitiesCore::unicodeKeyUp(wchar_t key)
{
    WORD wk = static_cast<WORD>(key);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan = wk;
    input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        removeEvent(Event(IU_TYPE::IU_UC, wk));

    return success;
}

bool InputUtilitiesCore::scKeyDown(wchar_t key)
{
    BYTE vk = VkKeyScanW(key);
    if (vk == 0xFF)
        return false;

    WORD scancode = MapVirtualKeyEx(vk & 0xFF, MAPVK_VK_TO_VSC, GetKeyboardLayout(0));

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scancode;               
    input.ki.dwFlags = KEYEVENTF_SCANCODE;   
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    bool success = SendInput(1, &input, sizeof(INPUT));

    if (success)
        this->runningInputs.push_back({ IU_TYPE::IU_SCK, scancode });

    return success;
}

bool InputUtilitiesCore::scKeyUp(wchar_t key)
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

    if (success)
        removeEvent(Event(IU_TYPE::IU_SCK, scancode));

    return success;
}

bool InputUtilitiesCore::keyDown(Event e)
{
    bool success = true;

    switch (e.type) {
    case IU_TYPE::IU_VK:
        success &= this->vKeyDown(e.iu_event);
        break;
    case IU_TYPE::IU_UC:
        success &= this->unicodeKeyDown(e.iu_event);
        break;
    case IU_TYPE::IU_SCK:
        success &= this->scKeyDown(e.iu_event);
        break;
    default:
        success = false;
    }

    return success;
}

bool InputUtilitiesCore::keyUp(Event e)
{
    bool success = true;

    switch (e.type) {
    case IU_TYPE::IU_VK:
        success &= this->vKeyUp(e.iu_event);
    case IU_TYPE::IU_UC:
        success &= this->unicodeKeyUp(e.iu_event);
    case IU_TYPE::IU_SCK:
        success &= this->scKeyUp(e.iu_event);
        break;
    default:
        success = false;
    }

    return success;
}

bool InputUtilitiesCore::vkMultiKeyDown(const std::vector<WORD>& vkCodes)
{
    bool success = true;

    for (const auto& vk : vkCodes) {
        success &= this->vKeyDown(vk);
        if (success)
            this->runningInputs.push_back({ IU_TYPE::IU_VK, vk });
        else
            break;
    }

    return success;
}

bool InputUtilitiesCore::vkMultiKeyUp(const std::vector<WORD>& vkCodes)
{
    bool success = true;

    for (const auto& vk : vkCodes) {
        success &= this->vKeyUp(vk);
        if (success)
            removeEvent(Event(IU_TYPE::IU_VK, vk));
    }

    return success;
}

bool InputUtilitiesCore::unicodeMultiKeyDown(const std::vector<wchar_t>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        success &= this->unicodeKeyDown(key);
        if (success)
            this->runningInputs.push_back({ IU_TYPE::IU_UC, static_cast<WORD>(key) });
        else
            break;
    }

    return success;
}

bool InputUtilitiesCore::unicodeMultiKeyUp(const std::vector<wchar_t>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        success &= this->unicodeKeyUp(key);
        if (success)
            removeEvent(Event(IU_TYPE::IU_UC, static_cast<WORD>(key)));
    }

    return success;
}

bool InputUtilitiesCore::scMultiKeyDown(const std::vector<wchar_t>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        success &= this->scKeyDown(key);
        if (success)
            this->runningInputs.push_back({ IU_TYPE::IU_SCK, static_cast<WORD>(key) });
        else
            break;
    }

    return success;
}

bool InputUtilitiesCore::scMultiKeyUp(const std::vector<wchar_t>& keys)
{
    bool success = true;

    for (const auto& key : keys) {
        success &= this->scKeyUp(key);
        if (success)
            removeEvent({ IU_TYPE::IU_SCK, static_cast<WORD>(key) });
        else
            break;
    }

    return success;
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

bool InputUtilitiesCore::removeEvent(const Event& e)
{
    auto originalSize = runningInputs.size();

    this->runningInputs.erase(
        std::remove_if(runningInputs.begin(), runningInputs.end(),
            [&e](const Event& ev) {
                return ev.type == e.type && ev.iu_event == e.iu_event;
            }),
        runningInputs.end()
    );

    return runningInputs.size() < originalSize;
}

void InputUtilitiesCore::reset()
{
    for (const auto& input : runningInputs)
    {
        switch (input.type)
        {
        case IU_TYPE::IU_VK:
            vKeyUp(input.iu_event);
            break;
        case IU_TYPE::IU_UC:
            unicodeKeyUp(input.iu_event);
            break;
        case IU_TYPE::IU_SCK:
            scKeyUp(input.iu_event);
            break;
        case IU_TYPE::IU_MOUSE:
            MouseEvent(input.iu_event << 1); //Bit shift left to obtain UP equivalence
            break;
        }
    }
}