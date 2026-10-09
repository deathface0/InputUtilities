#pragma once

#include <windows.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <inpututil/Backend.h>

// Test double for inpututil::Backend: records injected events instead of
// sending them, simulates failures and runs on a fake clock that only moves
// when someone sleeps, so timing tests are instant and deterministic.
class FakeBackend : public inpututil::Backend {
public:
    // --- Recording and failure simulation ---------------------------------
    std::vector<std::vector<INPUT>> batches;  // one entry per sendInput call (accepted events only)
    std::optional<std::size_t> acceptLimit;   // max events accepted per call
    std::uint32_t errorCode = 0;              // returned by lastError()
    std::optional<std::size_t> rejectAfter;   // calls accepted before every later one is rejected
    std::vector<Clock::time_point> batchTimes; // fake clock at each sendInput call

    // --- Screen and cursor --------------------------------------------------
    inpututil::Point cursor{0, 0}; // moved by the mouse events that get sent
    inpututil::Rect screen{0, 0, 1920, 1080};
    bool cursorAvailable = true;
    bool moveCursor = true; // false: the cursor ignores moves (a target that is never reached)

    /// How absolute coordinates (0..65535) become pixels. Windows does not
    /// document it; these are the two models seen in practice.
    enum class AbsoluteModel { A /* n * w / 65536 */, B /* n * (w - 1) / 65535 */ };
    AbsoluteModel absoluteModel = AbsoluteModel::A;

    // --- Keyboard layout (filled by each test) ------------------------------
    std::map<wchar_t, std::int16_t> vkScan;          // character -> VkKeyScan value
    std::map<std::uint16_t, std::uint16_t> vkToScan; // vk -> scan code (0xE0xx for extended)

    // --- Key state ----------------------------------------------------------
    std::set<std::uint16_t> keysDown;
    std::map<std::uint16_t, Clock::time_point> pressedFrom; // key counts as down from that fake time on
    std::set<std::uint16_t> toggled;

    // --- Time ---------------------------------------------------------------
    Clock::time_point clock{};
    std::vector<Clock::duration> sleeps;
    Clock::duration sleepOvershoot{};         // how late every sleep wakes up
    std::chrono::milliseconds doubleClick{500};

    /// vk -> scan code table of a US keyboard exactly as MapVirtualKeyEx
    /// (MAPVK_VK_TO_VSC_EX) reports it on Windows 10, quirks included:
    /// navigation keys come without the 0xE0 prefix, PrintScreen is 0x54 and
    /// Pause is 0xE11D.
    void loadUsScanCodes() {
        const char* letters = "QWERTYUIOP";
        for (int i = 0; i < 10; ++i) vkToScan[static_cast<std::uint16_t>(letters[i])] = static_cast<std::uint16_t>(0x10 + i);
        letters = "ASDFGHJKL";
        for (int i = 0; i < 9; ++i) vkToScan[static_cast<std::uint16_t>(letters[i])] = static_cast<std::uint16_t>(0x1E + i);
        letters = "ZXCVBNM";
        for (int i = 0; i < 7; ++i) vkToScan[static_cast<std::uint16_t>(letters[i])] = static_cast<std::uint16_t>(0x2C + i);
        for (int i = 1; i <= 9; ++i) vkToScan[static_cast<std::uint16_t>('0' + i)] = static_cast<std::uint16_t>(0x01 + i);
        vkToScan['0'] = 0x0B;
        for (int i = 0; i < 10; ++i) vkToScan[static_cast<std::uint16_t>(VK_F1 + i)] = static_cast<std::uint16_t>(0x3B + i);
        vkToScan[VK_F11] = 0x57;
        vkToScan[VK_F12] = 0x58;

        vkToScan[VK_ESCAPE] = 0x01;
        vkToScan[VK_BACK] = 0x0E;
        vkToScan[VK_TAB] = 0x0F;
        vkToScan[VK_RETURN] = 0x1C;
        vkToScan[VK_SPACE] = 0x39;
        vkToScan[VK_CAPITAL] = 0x3A;
        vkToScan[VK_OEM_MINUS] = 0x0C;

        vkToScan[VK_SHIFT] = 0x2A;
        vkToScan[VK_CONTROL] = 0x1D;
        vkToScan[VK_MENU] = 0x38;
        vkToScan[VK_LSHIFT] = 0x2A;
        vkToScan[VK_RSHIFT] = 0x36;
        vkToScan[VK_LCONTROL] = 0x1D;
        vkToScan[VK_RCONTROL] = 0xE01D;
        vkToScan[VK_LMENU] = 0x38;
        vkToScan[VK_RMENU] = 0xE038;
        vkToScan[VK_LWIN] = 0xE05B;
        vkToScan[VK_APPS] = 0xE05D;

        vkToScan[VK_LEFT] = 0x4B;
        vkToScan[VK_UP] = 0x48;
        vkToScan[VK_RIGHT] = 0x4D;
        vkToScan[VK_DOWN] = 0x50;
        vkToScan[VK_INSERT] = 0x52;
        vkToScan[VK_DELETE] = 0x53;
        vkToScan[VK_HOME] = 0x47;
        vkToScan[VK_END] = 0x4F;
        vkToScan[VK_PRIOR] = 0x49;
        vkToScan[VK_NEXT] = 0x51;
        vkToScan[VK_NUMLOCK] = 0x45;
        vkToScan[VK_DIVIDE] = 0xE035;
        vkToScan[VK_SNAPSHOT] = 0x54;
        vkToScan[VK_PAUSE] = 0xE11D;
        vkToScan[VK_VOLUME_UP] = 0xE030;
    }

    /// Characters of a US layout as VkKeyScanEx reports them on Windows 10
    /// ('A' = 0x141 Shift+A, '_' = 0x1BD, '@' = 0x132; no key for 'ñ' or '€').
    void loadUsLayout() {
        loadUsScanCodes();
        loadLettersAndDigits();
        vkScan[L'!'] = 0x0131;
        vkScan[L'@'] = 0x0132;
        vkScan[L'-'] = 0x00BD;
        vkScan[L'_'] = 0x01BD;
    }

    /// A shared fake with the US layout loaded, the usual backend of a test fixture.
    static std::shared_ptr<FakeBackend> withUsLayout() {
        auto fake = std::make_shared<FakeBackend>();
        fake->loadUsLayout();
        return fake;
    }

    /// Characters of a Spanish layout: '@' and '€' need AltGr (Ctrl+Alt),
    /// 'ñ' has its own key and 'á' is a dead-key composition (no single key).
    void loadEsLayout() {
        loadUsScanCodes();
        loadLettersAndDigits();
        vkToScan[VK_OEM_3] = 0x27;     // ñ
        vkToScan[VK_OEM_MINUS] = 0x35; // - and _
        vkScan[L'!'] = 0x0131;
        vkScan[L'@'] = 0x0632;
        vkScan[L'€'] = 0x0645;
        vkScan[L'ñ'] = 0x00C0;
        vkScan[L'Ñ'] = 0x01C0;
        vkScan[L'-'] = 0x00BD;
        vkScan[L'_'] = 0x01BD;
    }

    /// Every accepted event, in order, across all batches.
    std::vector<INPUT> allSent() const {
        std::vector<INPUT> all;
        for (const auto& batch : batches) all.insert(all.end(), batch.begin(), batch.end());
        return all;
    }

    unsigned sendInput(std::span<const tagINPUT> inputs) override {
        std::size_t accepted = std::min(inputs.size(), acceptLimit.value_or(inputs.size()));
        if (rejectAfter && batchTimes.size() >= *rejectAfter) accepted = 0;
        batchTimes.push_back(clock);
        batches.emplace_back(inputs.begin(), inputs.begin() + static_cast<std::ptrdiff_t>(accepted));
        for (std::size_t i = 0; i < accepted; ++i) applyMove(inputs[i]);
        return static_cast<unsigned>(accepted);
    }

    std::uint32_t lastError() override { return errorCode; }

    std::optional<inpututil::Point> cursorPos() override {
        if (!cursorAvailable) return std::nullopt;
        return cursor;
    }

    inpututil::Rect virtualScreen() override { return screen; }

    std::int16_t vkKeyScan(wchar_t ch) override {
        const auto it = vkScan.find(ch);
        return it == vkScan.end() ? std::int16_t{-1} : it->second;
    }

    std::uint16_t vkToScanCode(std::uint16_t vk) override {
        const auto it = vkToScan.find(vk);
        return it == vkToScan.end() ? std::uint16_t{0} : it->second;
    }

    std::uint16_t scanCodeToVk(std::uint16_t scanCode) override {
        for (const auto& [vk, sc] : vkToScan)
            if (sc == scanCode) return vk;
        return 0;
    }

    bool isKeyDown(std::uint16_t vk) override {
        if (keysDown.contains(vk)) return true;
        const auto it = pressedFrom.find(vk);
        return it != pressedFrom.end() && clock >= it->second;
    }

    bool isKeyToggled(std::uint16_t vk) override { return toggled.contains(vk); }

    Clock::time_point now() override { return clock; }

    void sleepUntil(Clock::time_point deadline) override {
        sleeps.push_back(deadline > clock ? deadline - clock : Clock::duration::zero());
        if (deadline > clock) clock = deadline + sleepOvershoot;
    }

    std::chrono::milliseconds doubleClickTime() override { return doubleClick; }

private:
    int toPixel(LONG normalized, int origin, int extent) const {
        const long long n = normalized;
        if (absoluteModel == AbsoluteModel::A) return origin + static_cast<int>(n * extent / 65536);
        return origin + static_cast<int>(n * (extent - 1) / 65535);
    }

    void applyMove(const INPUT& in) {
        if (!moveCursor || in.type != INPUT_MOUSE || !(in.mi.dwFlags & MOUSEEVENTF_MOVE)) return;
        if (in.mi.dwFlags & MOUSEEVENTF_ABSOLUTE) {
            cursor = {toPixel(in.mi.dx, screen.left, screen.width), toPixel(in.mi.dy, screen.top, screen.height)};
        } else {
            cursor.x = std::clamp(cursor.x + static_cast<int>(in.mi.dx), screen.left, screen.left + screen.width - 1);
            cursor.y = std::clamp(cursor.y + static_cast<int>(in.mi.dy), screen.top, screen.top + screen.height - 1);
        }
    }

    void loadLettersAndDigits() {
        for (wchar_t c = L'a'; c <= L'z'; ++c) vkScan[c] = static_cast<std::int16_t>(c - L'a' + 'A');
        for (wchar_t c = L'A'; c <= L'Z'; ++c) vkScan[c] = static_cast<std::int16_t>(0x0100 | c);
        for (wchar_t c = L'0'; c <= L'9'; ++c) vkScan[c] = static_cast<std::int16_t>(c);
        vkScan[L' '] = VK_SPACE;
    }
};
