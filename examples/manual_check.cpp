// Guided checks with the real mouse and keyboard: what the unit tests (which
// use a fake backend) cannot confirm. Run it yourself; it moves the cursor and
// types into the focused window.
#include "ExampleSupport.h"

#include <iostream>
#include <string>
#include <vector>

using namespace inpututil;
using namespace std::chrono_literals;

namespace {

struct Summary {
    int passed = 0;
    int failed = 0;
    void record(bool ok) { (ok ? passed : failed)++; }
};

bool askYesNo(const char* question) {
    std::printf("%s [y/n] ", question);
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y' || answer[0] == 's' || answer[0] == 'S');
}

void waitForEnter(const char* message) {
    std::printf("%s (Enter to continue) ", message);
    std::string ignored;
    std::getline(std::cin, ignored);
}

std::vector<RECT> monitors() {
    std::vector<RECT> rects;
    EnumDisplayMonitors(
        nullptr, nullptr,
        [](HMONITOR, HDC, LPRECT rect, LPARAM data) -> BOOL {
            reinterpret_cast<std::vector<RECT>*>(data)->push_back(*rect);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&rects));
    return rects;
}

// 1) The cursor must land on the exact pixel on every monitor.
void checkCursor(Input& input, Summary& summary) {
    const auto start = input.mouse.position();
    std::vector<Point> targets;
    for (const RECT& m : monitors()) {
        targets.push_back({m.left, m.top});
        targets.push_back({m.right - 1, m.top});
        targets.push_back({m.left, m.bottom - 1});
        targets.push_back({m.right - 1, m.bottom - 1});
        targets.push_back({(m.left + m.right) / 2, (m.top + m.bottom) / 2});
    }
    targets.push_back({1000, 500}); // one pixel short in v1

    int ok = 0;
    for (const Point target : targets) {
        const Status status = input.mouse.moveTo(target);
        const auto reached = input.mouse.position(); // no pause: moveTo waits for Windows to apply the move
        const bool pass = status && reached && *reached == target;
        ok += pass ? 1 : 0;
        std::printf("  target (%6d, %6d)  reached (%6d, %6d)  %s\n", target.x, target.y,
                    reached ? reached->x : 0, reached ? reached->y : 0, pass ? "PASS" : "FAIL");
    }

    const Point smoothTarget{(targets.front().x + targets.back().x) / 2,
                             (targets.front().y + targets.back().y) / 2};
    input.mouse.moveTo(smoothTarget, Motion::smooth(300ms));
    const bool smoothOk = input.mouse.position() == smoothTarget;
    std::printf("  smooth move to (%d, %d)            %s\n", smoothTarget.x, smoothTarget.y,
                smoothOk ? "PASS" : "FAIL");

    if (start) input.mouse.moveTo(*start);
    const bool pass = ok == static_cast<int>(targets.size()) && smoothOk;
    std::printf("Cursor: %d/%zu exact. %s\n", ok, targets.size(), pass ? "PASS" : "FAIL");
    summary.record(pass);
}

// 2) Unicode and real keystrokes must produce the same text in an application.
void checkTyping(Input& input, Summary& summary) {
    examples::countdown("Focus an empty text editor (e.g. Notepad)", 5);
    input.keyboard.type("Unicode:    PlayerOne_123 @ñ€ 😀\n");
    input.keyboard.type("Keystrokes: PlayerOne_123 @ñ€ 😀\n", {.mode = TextMode::Keystrokes});
    summary.record(askYesNo("Do both lines read exactly 'PlayerOne_123 @ñ€ 😀'?"));
}

// 3) Raw relative movement: the distance depends on pointer speed settings.
void checkRaw(Input& input) {
    const auto before = input.mouse.position();
    input.mouse.moveRaw(200, 0);
    std::this_thread::sleep_for(50ms);
    const auto after = input.mouse.position();
    if (before && after)
        std::printf("moveRaw(200, 0) moved the cursor %d px horizontally (depends on pointer speed and "
                    "'Enhance pointer precision'; informative only).\n",
                    after->x - before->x);
    input.mouse.moveRaw(-200, 0);
}

// 4) F12 must stop a long operation and leave nothing held.
void checkAbort(Input& input, Summary& summary) {
    input.setAbortKey(Key::F12);
    std::puts("Slow typing will start; press F12 while it types.");
    examples::countdown("Focus a text editor", 5);
    const Status status = input.keyboard.type(
        "This line is typed slowly so that you can press F12 in the middle of it.", {.delay = 120ms});
    const bool pass = status == Error::Aborted && input.heldCount() == 0;
    std::printf("Result: %s, held keys: %zu. %s\n", status.message().c_str(), input.heldCount(),
                pass ? "PASS" : "FAIL");
    summary.record(pass);
    input.setAbortKey(std::nullopt);
}

// 5) Closing the console must not leave Shift stuck.
[[noreturn]] void checkEmergency(Input& input) {
    installEmergencyRelease();
    input.keyboard.down(Key::Shift);
    std::puts("Shift is now held by this program.");
    std::puts("Close this console window with the X. Then type in another application:");
    std::puts("letters must come out in lowercase (Shift was released).");
    while (true) std::this_thread::sleep_for(1s);
}

} // namespace

int main() {
    examples::setUp();
    Input input;
    Summary summary;

    std::puts("inpututil manual check - this program moves the mouse and types for real.");
    while (true) {
        std::puts("\n1) Cursor accuracy on every monitor");
        std::puts("2) Typing (Unicode and keystrokes) in an editor");
        std::puts("3) Raw relative movement (moveRaw)");
        std::puts("4) Abort key (F12) during slow typing");
        std::puts("5) Emergency release when the console is closed");
        std::puts("0) Exit");
        std::printf("> ");

        std::string choice;
        if (!std::getline(std::cin, choice) || choice == "0") break;
        if (choice == "1") checkCursor(input, summary);
        else if (choice == "2") checkTyping(input, summary);
        else if (choice == "3") checkRaw(input);
        else if (choice == "4") checkAbort(input, summary);
        else if (choice == "5") {
            waitForEnter("This test ends with you closing the console.");
            checkEmergency(input);
        }
    }

    std::printf("\nSummary: %d passed, %d failed.\n", summary.passed, summary.failed);
    return summary.failed == 0 ? 0 : 1;
}
