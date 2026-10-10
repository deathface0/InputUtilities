#include "TestSupport.h"

#include "FakeBackend.h"

#include <algorithm>

#include <inpututil/inpututil.h>

using inpututil::Config;
using inpututil::Input;
using inpututil::Key;

namespace {

std::size_t countReleases(const FakeBackend& fake) {
    const auto sent = fake.allSent();
    return static_cast<std::size_t>(std::count_if(sent.begin(), sent.end(), [](const INPUT& in) {
        return in.type == INPUT_KEYBOARD && (in.ki.dwFlags & KEYEVENTF_KEYUP) != 0;
    }));
}

} // namespace

TEST_CASE("emergencyReleaseAll releases what every live Input holds") {
    auto first = std::make_shared<FakeBackend>();
    auto second = std::make_shared<FakeBackend>();
    Input a(Config{.backend = first});
    Input b(Config{.backend = second});
    REQUIRE(a.keyboard.down(Key::Ctrl));
    REQUIRE(b.keyboard.down(Key::Shift));
    REQUIRE(b.keyboard.down(Key::A));

    inpututil::emergencyReleaseAll();
    CHECK(a.heldCount() == 0);
    CHECK(b.heldCount() == 0);
    CHECK(countReleases(*first) == 1);
    CHECK(countReleases(*second) == 2);
}

TEST_CASE("a destroyed Input is no longer reached by the emergency release") {
    auto fake = std::make_shared<FakeBackend>();
    {
        Input input(Config{.releaseOnDestroy = false, .backend = fake});
        REQUIRE(input.keyboard.down(Key::A));
    }
    const auto batches = fake->batches.size();
    inpututil::emergencyReleaseAll(); // would touch a dangling session if it were still registered
    CHECK(fake->batches.size() == batches);
}

TEST_CASE("a moved Input is released once") {
    auto fake = std::make_shared<FakeBackend>();
    Input source(Config{.backend = fake});
    REQUIRE(source.keyboard.down(Key::A));
    Input target = std::move(source);

    inpututil::emergencyReleaseAll();
    CHECK(countReleases(*fake) == 1);
    CHECK(target.heldCount() == 0);
}

TEST_CASE("installing the handlers twice is harmless") {
    inpututil::installEmergencyRelease();
    inpututil::installEmergencyRelease();
    CHECK(true);
}
