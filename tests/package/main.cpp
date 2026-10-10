// Uses the installed library without sending any input: a backend that only
// counts events stands in for Win32, while win32Backend() is still linked in.
#include <cstdio>

#include <inpututil/inpututil.h>

namespace {

class CountingBackend : public inpututil::Backend {
public:
    unsigned sent = 0;

    unsigned sendInput(std::span<const tagINPUT> inputs) override {
        sent += static_cast<unsigned>(inputs.size());
        return static_cast<unsigned>(inputs.size());
    }
    std::uint32_t lastError() override { return 0; }
    std::optional<inpututil::Point> cursorPos() override { return inpututil::Point{}; }
    inpututil::Rect virtualScreen() override { return {0, 0, 1920, 1080}; }
    std::int16_t vkKeyScan(wchar_t) override { return -1; }
    std::uint16_t vkToScanCode(std::uint16_t) override { return 0; }
    std::uint16_t scanCodeToVk(std::uint16_t) override { return 0; }
    bool isDeadKey(std::uint16_t, unsigned) override { return false; }
    bool isKeyDown(std::uint16_t) override { return false; }
    bool isKeyToggled(std::uint16_t) override { return false; }
    Clock::time_point now() override { return Clock::now(); }
    void sleepUntil(Clock::time_point) override {}
    std::chrono::milliseconds doubleClickTime() override { return std::chrono::milliseconds(500); }
};

} // namespace

int main() {
    const auto combo = inpututil::KeyCombo::parse("Ctrl+Shift+Esc");
    if (!combo || combo->toString() != "Ctrl+Shift+Esc") return 1;
    if (!inpututil::win32Backend()) return 2;

    auto backend = std::make_shared<CountingBackend>();
    inpututil::Input input(inpututil::Config{.backend = backend});
    if (!input.keyboard.press(*combo) || backend->sent != 6) return 3;

    std::puts("package OK");
    return 0;
}
