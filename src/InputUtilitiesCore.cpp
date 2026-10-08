#include "inpututil/InputUtilitiesCore.h"

InputUtilitiesCore::InputUtilitiesCore(bool safemode)
    : safemode(safemode)
{
    if (safemode) runningInputs.reserve(32);
}

InputUtilitiesCore::~InputUtilitiesCore()
{
    if (safemode)
        reset();
}

DWORD InputUtilitiesCore::GetMouseID(DWORD flags, DWORD mouseData)
{
    if (flags & (MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP)) return 1;
    if (flags & (MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP)) return 2;
    if (flags & (MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP)) return 3;
    if (flags & (MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP)) {
        if (mouseData == XBUTTON1) return 5;
        if (mouseData == XBUTTON2) return 6;
    }
    return 0;
}

DWORD InputUtilitiesCore::GetMouseUpFlag(DWORD id, DWORD& mouseData)
{
    mouseData = 0;

    switch (id) {
    case 1: return MOUSEEVENTF_LEFTUP;
    case 2: return MOUSEEVENTF_RIGHTUP;
    case 3: return MOUSEEVENTF_MIDDLEUP;
    case 5: mouseData = XBUTTON1; return MOUSEEVENTF_XUP;
    case 6: mouseData = XBUTTON2; return MOUSEEVENTF_XUP;
    default: return 0;
    }
}


InputResult InputUtilitiesCore::SetCursorPos(int x, int y, bool abs)
{
    POINT startPos;
    if (!GetCursorPos(&startPos)) return InputResult::SystemFailure;

    int targetX, targetY;

    if (abs) {
        targetX = x;
        targetY = y;
    }
    else {
        targetX = startPos.x + x;
        targetY = startPos.y + y;
    }

    // Screen metrics
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    if (screenW == 0 || screenH == 0) return InputResult::SystemFailure;

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.time = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE; // Always use ABSOLUTE move to avoid Win mouse acceleration inconsistency

    // Normalization
    input.mi.dx = (targetX * 65536) / screenW;
    input.mi.dy = (targetY * 65536) / screenH;

    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode) {
        POINT endPos;
        if (!GetCursorPos(&endPos)) return InputResult::SystemFailure;
        if (endPos.x == startPos.x && endPos.y == startPos.y) return InputResult::MouseSamePos;
    }

    return InputResult::Success;
}

InputResult InputUtilitiesCore::SetCursorPos(int x, int y, int steps, int delay, bool abs)
{
    POINT startPos;
    if (!GetCursorPos(&startPos)) return InputResult::SystemFailure;

    int startX = startPos.x;
    int startY = startPos.y;
    int endX = 0;
    int endY = 0;

    if (abs) {
        endX = x;
        endY = y;
    }
    else {
        endX = startX + x;
        endY = startY + y;
    }

    // Screen metrics
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    if (screenW == 0 || screenH == 0) return InputResult::SystemFailure;

    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.time = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE; // Always use ABSOLUTE move to avoid Win mouse acceleration inconsistency

    for (int i = 1; i <= steps; ++i)
    {
        double t = static_cast<double>(i) / steps;

        // SmoothStep (Ease-in-out)
        double smoothT = t * t * (3.0 - 2.0 * t);

        // Linear interpolation
        int currentX = startX + static_cast<int>((endX - startX) * smoothT);
        int currentY = startY + static_cast<int>((endY - startY) * smoothT);

        // Normalization
        input.mi.dx = (currentX * 65535) / screenW;
        input.mi.dy = (currentY * 65535) / screenH;

        if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

        if (delay > 0) Sleep(delay);
    }

    POINT finalPos;
    if (!GetCursorPos(&finalPos)) return InputResult::SystemFailure;
    if (finalPos.x == startX && finalPos.y == startY) return InputResult::MouseSamePos;

    return InputResult::Success;
}

InputResult InputUtilitiesCore::MouseEvent(DWORD dwFlags, DWORD mouseData) {
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = dwFlags;
    input.mi.mouseData = mouseData;

    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode) {
        DWORD id = GetMouseID(dwFlags, mouseData);

        if (id != 0) {
            bool isUp = (dwFlags & (MOUSEEVENTF_LEFTUP | MOUSEEVENTF_RIGHTUP | MOUSEEVENTF_MIDDLEUP | MOUSEEVENTF_XUP));
            if (isUp) runningInputs.erase({ InputType::Mouse, id });
            else      runningInputs.insert({ InputType::Mouse, id });
        }
    }
    return InputResult::Success;
}

InputResult InputUtilitiesCore::ExtraClickDown(DWORD xbutton)
{
    return MouseEvent(MOUSEEVENTF_XDOWN, xbutton);
}

InputResult InputUtilitiesCore::ExtraClickUp(DWORD xbutton)
{
    return MouseEvent(MOUSEEVENTF_XUP, xbutton);
}

InputResult InputUtilitiesCore::MouseWheelRoll(int scrolls, MWheelDir dir, UINT delta, MWheelAxis axis)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;

    int totalMovement = scrolls * static_cast<int>(dir) * static_cast<int>(delta);
    input.mi.mouseData = static_cast<DWORD>(totalMovement);
    input.mi.dwFlags = axis;

    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    return InputResult::Success;
}

InputResult InputUtilitiesCore::MouseWheelRoll(int scrolls, int delay, MWheelDir dir, UINT delta, MWheelAxis axis)
{
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.mouseData = (DWORD)(dir * delta);
    input.mi.dwFlags = axis;

    if (scrolls <= 0) return InputResult::Success;

    for (int i = 0; i < scrolls; i++) {
        if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;
        if (delay > 0) Sleep(delay);
    }

    return InputResult::Success;
}

InputResult InputUtilitiesCore::vKeyDown(WORD vkCode)
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = 0;
    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode) runningInputs.insert({ InputType::VK, (DWORD)vkCode });

    return InputResult::Success;
}

InputResult InputUtilitiesCore::vKeyUp(WORD vkCode)
{
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode)
        runningInputs.erase({ InputType::VK, (DWORD)vkCode });

    return InputResult::Success;
}

InputResult InputUtilitiesCore::unicodeKeyDown(wchar_t key)
{
    WORD wk = static_cast<WORD>(key);
    
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan = static_cast<WORD>(key);
    input.ki.dwFlags = KEYEVENTF_UNICODE;
    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode) runningInputs.insert({ InputType::UC, (DWORD)wk });

    return InputResult::Success;
}

InputResult InputUtilitiesCore::unicodeKeyUp(wchar_t key)
{
    WORD wk = static_cast<WORD>(key);

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    input.ki.wScan = wk;
    input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode) runningInputs.erase({ InputType::UC, (DWORD)wk });

    return InputResult::Success;
}

InputResult InputUtilitiesCore::scKeyDown(wchar_t key)
{
    SHORT scanResult = VkKeyScanW(key);
    if (scanResult == -1)  return InputResult::InvalidMapping;

    // VK -> ScanCode
    WORD vk = LOBYTE(scanResult);
    WORD scancode = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, GetKeyboardLayout(0));

    if (scancode == 0) return InputResult::InvalidMapping;

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scancode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode) runningInputs.insert({ InputType::SC, (DWORD)scancode });
    
    return InputResult::Success;
}

InputResult InputUtilitiesCore::scKeyUp(wchar_t key)
{
    SHORT scanResult = VkKeyScanW(key);
    if (scanResult == -1) return InputResult::InvalidMapping;

    // VK -> ScanCode
    WORD vk = LOBYTE(scanResult);
    WORD scancode = MapVirtualKeyEx(vk, MAPVK_VK_TO_VSC, GetKeyboardLayout(0));

    if (scancode == 0) return InputResult::InvalidMapping;

    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = scancode;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // Flag UP
    input.ki.dwExtraInfo = GetMessageExtraInfo();
    if (SendInput(1, &input, sizeof(INPUT)) == 0) return InputResult::SystemFailure;

    if (safemode) runningInputs.erase({ InputType::SC, (DWORD)scancode });

    return InputResult::Success;
}

InputResult InputUtilitiesCore::keyDown(Event e)
{
    InputResult InputResult;

    switch (e.type) {
    case InputType::VK:
        InputResult = vKeyDown(e.code);
        break;
    case InputType::UC:
        InputResult = unicodeKeyDown(e.code);
        break;
    case InputType::SC:
        InputResult = scKeyDown(e.code);
        break;
    default:
        InputResult = InputResult::InvalidMapping;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::keyUp(Event e)
{
    InputResult InputResult;

    switch (e.type) {
    case InputType::VK:
        InputResult = vKeyUp(e.code);
        break;
    case InputType::UC:
        InputResult = unicodeKeyUp(e.code);
        break;
    case InputType::SC:
        InputResult = scKeyUp(e.code);
        break;
    default:
        InputResult = InputResult::InvalidMapping;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::vkMultiKeyDown(const std::vector<WORD>& vkCodes)
{
    InputResult InputResult = Success;

    for (const auto& vk : vkCodes) {
        InputResult = vKeyDown(vk);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::vkMultiKeyUp(const std::vector<WORD>& vkCodes)
{
    InputResult InputResult = Success;

    for (const auto& vk : vkCodes) {
        InputResult = vKeyUp(vk);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::unicodeMultiKeyDown(const std::vector<wchar_t>& keys)
{
    InputResult InputResult = Success;

    for (const auto& key : keys) {
        InputResult = unicodeKeyDown(key);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::unicodeMultiKeyUp(const std::vector<wchar_t>& keys)
{
    InputResult InputResult = Success;

    for (const auto& key : keys) {
        InputResult = unicodeKeyUp(key);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::scMultiKeyDown(const std::vector<wchar_t>& keys)
{
    InputResult InputResult = Success;

    for (const auto& key : keys) {
        InputResult = scKeyDown(key);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::scMultiKeyUp(const std::vector<wchar_t>& keys)
{
    InputResult InputResult = Success;

    for (const auto& key : keys) {
        InputResult = scKeyUp(key);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::multiKeyDown(const std::vector<Event>& keys)
{
    InputResult InputResult = Success;

    for (const auto& key : keys) {
        InputResult = keyDown(key);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

InputResult InputUtilitiesCore::multiKeyUp(const std::vector<Event>& keys)
{
    InputResult InputResult = Success;

    for (const auto& key : keys) {
        InputResult = keyUp(key);
        if (InputResult != InputResult::Success)
            break;
    }

    return InputResult;
}

void InputUtilitiesCore::reset()
{
    if (runningInputs.empty()) return;

    std::vector<INPUT> inputs;
    inputs.reserve(runningInputs.size());

    for (const auto& event : runningInputs)
    {
        INPUT input = { 0 };
        switch (event.type)
        {
        case InputType::VK:
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = (WORD)event.code;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            break;
        case InputType::SC:
            input.type = INPUT_KEYBOARD;
            input.ki.wScan = (WORD)event.code;
            input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
            break;
        case InputType::UC:
            input.type = INPUT_KEYBOARD;
            input.ki.wScan = (WORD)event.code;
            input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            break;
        case InputType::Mouse:
            input.type = INPUT_MOUSE;
            DWORD mouseData = 0;
            input.mi.dwFlags = GetMouseUpFlag(event.code, mouseData);
            input.mi.mouseData = mouseData;
            break;
        }

        if (input.type != 0) {
            inputs.push_back(input);
        }
    }

    if (!inputs.empty()) {
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    }

    runningInputs.clear();
}