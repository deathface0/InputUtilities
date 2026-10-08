#pragma once

#include <windows.h>

#include <algorithm>
#include <map>
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

    // --- Screen and cursor --------------------------------------------------
    inpututil::Point cursor{0, 0};
    inpututil::Rect screen{0, 0, 1920, 1080};
    bool cursorAvailable = true;

    // --- Keyboard layout (filled by each test) ------------------------------
    std::map<wchar_t, std::int16_t> vkScan;          // character -> VkKeyScan value
    std::map<std::uint16_t, std::uint16_t> vkToScan; // vk -> scan code (0xE0xx for extended)

    // --- Key state ----------------------------------------------------------
    std::set<std::uint16_t> keysDown;
    std::set<std::uint16_t> toggled;

    // --- Time ---------------------------------------------------------------
    Clock::time_point clock{};
    std::vector<Clock::duration> sleeps;
    std::chrono::milliseconds doubleClick{500};

    /// Every accepted event, in order, across all batches.
    std::vector<INPUT> allSent() const {
        std::vector<INPUT> all;
        for (const auto& batch : batches) all.insert(all.end(), batch.begin(), batch.end());
        return all;
    }

    unsigned sendInput(std::span<const tagINPUT> inputs) override {
        const std::size_t accepted = std::min(inputs.size(), acceptLimit.value_or(inputs.size()));
        batches.emplace_back(inputs.begin(), inputs.begin() + static_cast<std::ptrdiff_t>(accepted));
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

    bool isKeyDown(std::uint16_t vk) override { return keysDown.contains(vk); }

    bool isKeyToggled(std::uint16_t vk) override { return toggled.contains(vk); }

    Clock::time_point now() override { return clock; }

    void sleepUntil(Clock::time_point deadline) override {
        sleeps.push_back(deadline > clock ? deadline - clock : Clock::duration::zero());
        clock = std::max(clock, deadline);
    }

    std::chrono::milliseconds doubleClickTime() override { return doubleClick; }
};
