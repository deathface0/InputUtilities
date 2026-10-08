#include "TestSupport.h"

#include "FakeBackend.h"

using namespace std::chrono_literals;

namespace {

INPUT keyInput(WORD vk) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    return in;
}

} // namespace

TEST_CASE("FakeBackend records every batch separately") {
    FakeBackend fake;
    const INPUT first[] = {keyInput('A'), keyInput('B')};
    const INPUT second[] = {keyInput('C')};

    CHECK(fake.sendInput(first) == 2);
    CHECK(fake.sendInput(second) == 1);

    REQUIRE(fake.batches.size() == 2);
    CHECK(fake.batches[0].size() == 2);
    CHECK(fake.batches[1][0].ki.wVk == 'C');
    CHECK(fake.allSent().size() == 3);
}

TEST_CASE("FakeBackend can reject part of a batch") {
    FakeBackend fake;
    fake.acceptLimit = 2;
    fake.errorCode = ERROR_ACCESS_DENIED;
    const INPUT batch[] = {keyInput('A'), keyInput('B'), keyInput('C')};

    CHECK(fake.sendInput(batch) == 2);
    CHECK(fake.allSent().size() == 2);
    CHECK(fake.lastError() == ERROR_ACCESS_DENIED);
}

TEST_CASE("FakeBackend clock only moves when sleeping") {
    FakeBackend fake;
    const auto start = fake.now();
    CHECK(fake.now() == start);

    fake.sleepUntil(start + 30ms);
    CHECK(fake.now() == start + 30ms);

    fake.sleepUntil(start + 10ms); // deadline in the past: the clock never goes back
    CHECK(fake.now() == start + 30ms);

    REQUIRE(fake.sleeps.size() == 2);
    CHECK(fake.sleeps[0] == 30ms);
    CHECK(fake.sleeps[1] == 0ms);
}

TEST_CASE("FakeBackend layout maps are configurable") {
    FakeBackend fake;
    CHECK(fake.vkKeyScan(L'a') == -1);
    CHECK(fake.vkToScanCode(VK_LEFT) == 0);

    fake.vkScan[L'A'] = 0x0141; // Shift + VK 'A'
    fake.vkToScan[VK_LEFT] = 0xE04B;
    CHECK(fake.vkKeyScan(L'A') == 0x0141);
    CHECK(fake.vkToScanCode(VK_LEFT) == 0xE04B);
    CHECK(fake.scanCodeToVk(0xE04B) == VK_LEFT);
}

// Only read-only Win32 calls: these tests never inject input.
TEST_CASE("win32Backend is a shared instance with sane system values") {
    const auto backend = inpututil::win32Backend();
    REQUIRE(backend != nullptr);
    CHECK(backend == inpututil::win32Backend());

    const inpututil::Rect screen = backend->virtualScreen();
    CHECK(screen.width > 0);
    CHECK(screen.height > 0);

    CHECK(backend->vkToScanCode('A') != 0);
    CHECK(backend->doubleClickTime() > 0ms);
}

TEST_CASE("win32Backend sleeps until the deadline") {
    const auto backend = inpututil::win32Backend();
    const auto start = backend->now();
    backend->sleepUntil(start + 5ms);
    const auto elapsed = backend->now() - start;

    CHECK(elapsed >= 5ms);
    CHECK(elapsed < 100ms); // generous: CI machines can be slow
    CHECK(backend->now() >= start);
}
