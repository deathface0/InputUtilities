#include "TestSupport.h"

#include "FakeBackend.h"
#include "detail/Session.h"

#include <thread>

#include <inpututil/Input.h>

using inpututil::Error;
using inpututil::detail::SendMode;
using inpututil::detail::Session;
using namespace std::chrono_literals;

namespace {

constexpr std::uintptr_t kTag = inpututil::kDefaultExtraInfoTag;

INPUT key(WORD vk, WORD scan, DWORD flags = 0) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = vk;
    in.ki.wScan = scan;
    in.ki.dwFlags = flags;
    return in;
}

INPUT keyDown(WORD vk, WORD scan) { return key(vk, scan); }
INPUT keyUp(WORD vk, WORD scan) { return key(vk, scan, KEYEVENTF_KEYUP); }

INPUT mouse(DWORD flags, DWORD data = 0) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = flags;
    in.mi.mouseData = data;
    return in;
}

struct Fixture {
    std::shared_ptr<FakeBackend> fake = std::make_shared<FakeBackend>();
    Session session{fake, kTag};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "send tags events without dwExtraInfo and keeps custom values") {
    INPUT custom = keyDown('B', 0x30);
    custom.ki.dwExtraInfo = 1234;
    const INPUT batch[] = {keyDown('A', 0x1E), custom, mouse(MOUSEEVENTF_MOVE)};

    REQUIRE(session.send(batch));
    const auto sent = fake->allSent();
    CHECK(sent[0].ki.dwExtraInfo == kTag);
    CHECK(sent[1].ki.dwExtraInfo == 1234);
    CHECK(sent[2].mi.dwExtraInfo == kTag);
}

TEST_CASE_FIXTURE(Fixture, "a key is tracked while held and only once") {
    const INPUT down[] = {keyDown('A', 0x1E)};
    const INPUT up[] = {keyUp('A', 0x1E)};

    REQUIRE(session.send(down));
    CHECK(session.heldCount() == 1);
    REQUIRE(session.send(down));
    CHECK(session.heldCount() == 1);
    REQUIRE(session.send(up));
    CHECK(session.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "virtual-key press and scan-code release are the same physical key") {
    const INPUT down[] = {keyDown('A', 0x1E)};
    const INPUT up[] = {key(0, 0x1E, KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP)};

    REQUIRE(session.send(down));
    REQUIRE(session.send(up));
    CHECK(session.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "releaseAll releases in reverse order in a single batch") {
    const INPUT downs[] = {keyDown('A', 0x1E), keyDown('B', 0x30), key(VK_LEFT, 0x4B, KEYEVENTF_EXTENDEDKEY)};
    REQUIRE(session.send(downs));
    fake->batches.clear();

    REQUIRE(session.releaseAll());
    REQUIRE(fake->batches.size() == 1);
    const auto& releases = fake->batches[0];
    REQUIRE(releases.size() == 3);

    CHECK(releases[0].ki.wVk == VK_LEFT);
    CHECK(releases[0].ki.wScan == 0x4B);
    CHECK(releases[0].ki.dwFlags == (KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP));
    CHECK(releases[1].ki.wVk == 'B');
    CHECK(releases[2].ki.wVk == 'A');
    CHECK(releases[2].ki.dwFlags == KEYEVENTF_KEYUP);
    CHECK(session.heldCount() == 0);

    REQUIRE(session.releaseAll()); // nothing held: nothing sent
    CHECK(fake->batches.size() == 1);
}

TEST_CASE_FIXTURE(Fixture, "unicode keys are released as unicode") {
    const INPUT down[] = {key(0, L'ñ', KEYEVENTF_UNICODE)};
    REQUIRE(session.send(down));
    REQUIRE(session.releaseAll());

    const INPUT release = fake->allSent().back();
    CHECK(release.ki.wScan == L'ñ');
    CHECK(release.ki.dwFlags == (KEYEVENTF_UNICODE | KEYEVENTF_KEYUP));
}

TEST_CASE_FIXTURE(Fixture, "every mouse button bit is tracked") {
    SUBCASE("left and right in one event") {
        const INPUT down[] = {mouse(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_RIGHTDOWN)};
        REQUIRE(session.send(down));
        CHECK(session.heldCount() == 2);

        fake->batches.clear();
        REQUIRE(session.releaseAll());
        const auto& releases = fake->batches.at(0);
        REQUIRE(releases.size() == 2);
        CHECK(releases[0].mi.dwFlags == MOUSEEVENTF_RIGHTUP);
        CHECK(releases[1].mi.dwFlags == MOUSEEVENTF_LEFTUP);
    }
    SUBCASE("both extra buttons in one event") {
        const INPUT down[] = {mouse(MOUSEEVENTF_XDOWN, XBUTTON1 | XBUTTON2)};
        REQUIRE(session.send(down));
        CHECK(session.heldCount() == 2);

        const INPUT up[] = {mouse(MOUSEEVENTF_XUP, XBUTTON1)};
        REQUIRE(session.send(up));
        CHECK(session.heldCount() == 1);
    }
    SUBCASE("moves and wheel are not held") {
        const INPUT events[] = {mouse(MOUSEEVENTF_MOVE), mouse(MOUSEEVENTF_WHEEL, WHEEL_DELTA)};
        REQUIRE(session.send(events));
        CHECK(session.heldCount() == 0);
    }
}

TEST_CASE_FIXTURE(Fixture, "a partially accepted batch reports PartialSend and tracks only what was sent") {
    fake->acceptLimit = 1;
    fake->errorCode = ERROR_ACCESS_DENIED;
    const INPUT downs[] = {keyDown('A', 0x1E), keyDown('B', 0x30)};

    const auto st = session.send(downs);
    CHECK(st == Error::PartialSend);
    CHECK(st.win32Error() == ERROR_ACCESS_DENIED);
    CHECK(session.heldCount() == 1);
    CHECK(fake->allSent().size() == 1);
}

TEST_CASE_FIXTURE(Fixture, "best effort retries rejected events one by one") {
    const INPUT ups[] = {keyUp('A', 0x1E), keyUp('B', 0x30), keyUp('C', 0x2E)};

    SUBCASE("everything gets through eventually") {
        fake->acceptLimit = 1;
        CHECK(session.send(ups, SendMode::BestEffort));
        CHECK(fake->allSent().size() == 3);
        CHECK(fake->batches.size() == 3);
    }
    SUBCASE("the system rejects everything") {
        fake->acceptLimit = 0;
        fake->errorCode = ERROR_ACCESS_DENIED;
        const auto st = session.send(ups, SendMode::BestEffort);
        CHECK(st == Error::SystemFailure);
        CHECK(st.win32Error() == ERROR_ACCESS_DENIED);
    }
}

TEST_CASE_FIXTURE(Fixture, "keys that could not be released stay tracked for a retry") {
    const INPUT downs[] = {keyDown('A', 0x1E), keyDown('B', 0x30)};
    REQUIRE(session.send(downs));

    fake->acceptLimit = 0;
    CHECK(session.releaseAll() == Error::SystemFailure);
    CHECK(session.heldCount() == 2);

    fake->acceptLimit.reset();
    CHECK(session.releaseAll());
    CHECK(session.heldCount() == 0);
}

TEST_CASE_FIXTURE(Fixture, "release only sends what is still held") {
    const INPUT down[] = {keyDown('A', 0x1E)};
    REQUIRE(session.send(down));
    fake->batches.clear();

    const INPUT ups[] = {keyUp('A', 0x1E), keyUp('B', 0x30), keyUp('A', 0x1E)};
    REQUIRE(session.release(ups));
    REQUIRE(fake->batches.size() == 1);
    REQUIRE(fake->batches[0].size() == 1); // B never went down, and A is released once
    CHECK(fake->batches[0][0].ki.wVk == 'A');
    CHECK(session.heldCount() == 0);

    REQUIRE(session.release(ups)); // nothing held any more: nothing sent
    CHECK(fake->batches.size() == 1);
}

TEST_CASE_FIXTURE(Fixture, "wait advances the backend clock") {
    const auto start = fake->now();
    REQUIRE(session.wait(30ms));
    CHECK(fake->now() - start == 30ms);
}

TEST_CASE_FIXTURE(Fixture, "concurrent sends never interleave batches") {
    constexpr int kThreads = 4;
    constexpr int kIterations = 200;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([this, t] {
            const WORD vk = static_cast<WORD>('A' + t);
            const WORD scan = static_cast<WORD>(0x10 + t);
            const INPUT pair[] = {keyDown(vk, scan), keyUp(vk, scan)};
            for (int i = 0; i < kIterations; ++i) session.send(pair);
        });
    }
    for (auto& thread : threads) thread.join();

    CHECK(session.heldCount() == 0);
    REQUIRE(fake->batches.size() == kThreads * kIterations);
    for (const auto& batch : fake->batches) {
        REQUIRE(batch.size() == 2);
        CHECK(batch[0].ki.wVk == batch[1].ki.wVk);
    }
}
