#include "detail/Session.h"

#include <algorithm>

namespace inpututil::detail {

namespace {

// Identity of a held input: kind in the high byte, value in the low bits.
constexpr std::uint32_t kKeyScan = 1u << 24;    // physical key: scan code | 0xE000 if extended
constexpr std::uint32_t kKeyVk = 2u << 24;      // key sent without scan code: virtual key
constexpr std::uint32_t kKeyUnicode = 3u << 24; // KEYEVENTF_UNICODE character
constexpr std::uint32_t kMouse = 4u << 24;      // mouse button index

struct MouseButtonFlags {
    DWORD down;
    DWORD up;
    DWORD data; // XBUTTON1/XBUTTON2 for the extra buttons, 0 otherwise
};

// clang-format off
constexpr MouseButtonFlags kMouseButtons[] = {
    {MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, 0},
    {MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, 0},
    {MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, 0},
    {MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON1},
    {MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, XBUTTON2},
};
// clang-format on

bool hasButton(const MOUSEINPUT& mi, DWORD flag, DWORD data) {
    if (!(mi.dwFlags & flag)) return false;
    return data == 0 || (mi.mouseData & data) != 0;
}

std::uint32_t keyIdentity(const KEYBDINPUT& ki) {
    if (ki.dwFlags & KEYEVENTF_UNICODE) return kKeyUnicode | ki.wScan;
    if (ki.wScan != 0) {
        const bool extended = (ki.dwFlags & KEYEVENTF_EXTENDEDKEY) || (ki.wScan & 0xFF00) == 0xE000;
        return kKeyScan | (extended ? 0xE000u : 0u) | (ki.wScan & 0xFFu);
    }
    return kKeyVk | ki.wVk;
}

// The key, or the first mouse button, that the event presses or releases; 0 for anything else.
std::uint32_t identityOf(const INPUT& event) {
    if (event.type == INPUT_KEYBOARD) return keyIdentity(event.ki);
    if (event.type == INPUT_MOUSE) {
        for (std::uint32_t i = 0; i < std::size(kMouseButtons); ++i) {
            const MouseButtonFlags& button = kMouseButtons[i];
            if (hasButton(event.mi, button.down, button.data) || hasButton(event.mi, button.up, button.data))
                return kMouse | i;
        }
    }
    return 0;
}

} // namespace

Session::Session(std::shared_ptr<Backend> backend, std::uintptr_t extraInfoTag)
    : backend_(std::move(backend)), extraInfoTag_(extraInfoTag) {
    registerSession(this);
}

Session::~Session() { unregisterSession(this); }

Status Session::send(std::span<const INPUT> inputs, SendMode mode) {
    std::lock_guard lock(mutex_);
    return sendLocked(inputs, mode);
}

Status Session::releaseAll() {
    std::lock_guard lock(mutex_);
    if (held_.empty()) return {};
    return sendLocked(releasesLocked(), SendMode::BestEffort);
}

void Session::emergencyRelease() noexcept {
    try {
        std::unique_lock lock(mutex_, std::defer_lock);
        if (lock.try_lock_for(std::chrono::milliseconds(50))) {
            if (!held_.empty()) sendLocked(releasesLocked(), SendMode::BestEffort);
            return;
        }
        // The lock is stuck (the crash happened while sending): send the
        // releases anyway, the process is going away.
        const std::vector<INPUT> releases = releasesLocked();
        if (!releases.empty()) backend_->sendInput(releases);
    } catch (...) {
        // Nothing else can be done while the process dies.
    }
}

std::vector<INPUT> Session::releasesLocked() const {
    std::vector<INPUT> releases;
    releases.reserve(held_.size());
    for (auto it = held_.rbegin(); it != held_.rend(); ++it) releases.push_back(it->release);
    return releases;
}

std::size_t Session::heldCount() const {
    std::lock_guard lock(mutex_);
    return held_.size();
}

bool Session::isHeld(const INPUT& event) const {
    const std::uint32_t id = identityOf(event);
    if (id == 0) return false;

    std::lock_guard lock(mutex_);
    return isHeldLocked(id);
}

Status Session::release(std::span<const INPUT> releases) {
    std::lock_guard lock(mutex_);
    std::vector<INPUT> stillHeld;
    std::vector<std::uint32_t> ids; // each key or button is released once
    for (const INPUT& event : releases) {
        const std::uint32_t id = identityOf(event);
        if (id == 0 || !isHeldLocked(id) || std::find(ids.begin(), ids.end(), id) != ids.end()) continue;
        stillHeld.push_back(event);
        ids.push_back(id);
    }
    return sendLocked(stillHeld, SendMode::BestEffort);
}

bool Session::isHeldLocked(std::uint32_t id) const {
    return std::any_of(held_.begin(), held_.end(), [id](const HeldInput& h) { return h.id == id; });
}

Status Session::checkAbort() {
    const std::uint16_t vk = abortVk_.load();
    if (!abortRequested_.load() && (vk == 0 || !backend_->isKeyDown(vk))) return {};
    releaseAll();
    return Error::Aborted;
}

Status Session::waitUntil(Backend::Clock::time_point deadline) {
    // Checked at least once, even when already late, and every 10 ms while waiting:
    // an abort can come from the key or from another thread at any time.
    constexpr auto kPollInterval = std::chrono::milliseconds(10);
    while (true) {
        if (const Status status = checkAbort(); !status) return status;
        const auto now = backend_->now();
        if (now >= deadline) return {};
        backend_->sleepUntil(std::min<Backend::Clock::time_point>(deadline, now + kPollInterval));
    }
}

Status Session::wait(std::chrono::nanoseconds duration) {
    return waitUntil(backend_->now() + std::chrono::duration_cast<Backend::Clock::duration>(duration));
}

Status Session::sendLocked(std::span<const INPUT> inputs, SendMode mode) {
    if (inputs.empty()) return {};

    std::vector<INPUT> batch(inputs.begin(), inputs.end());
    for (INPUT& input : batch) {
        if (input.type == INPUT_KEYBOARD && input.ki.dwExtraInfo == 0) input.ki.dwExtraInfo = extraInfoTag_;
        if (input.type == INPUT_MOUSE && input.mi.dwExtraInfo == 0) input.mi.dwExtraInfo = extraInfoTag_;
    }

    const std::size_t sent = std::min<std::size_t>(backend_->sendInput(batch), batch.size());
    for (std::size_t i = 0; i < sent; ++i) track(batch[i]);
    if (sent == batch.size()) return {};

    const std::uint32_t error = backend_->lastError();
    if (mode == SendMode::StopOnFailure)
        return {sent == 0 ? Error::SystemFailure : Error::PartialSend, error};

    // Best effort: retry each rejected event on its own so nothing stays held.
    std::size_t delivered = sent;
    for (std::size_t i = sent; i < batch.size(); ++i) {
        if (backend_->sendInput(std::span(&batch[i], 1)) == 1) {
            track(batch[i]);
            ++delivered;
        }
    }
    if (delivered == batch.size()) return {};
    return {delivered == 0 ? Error::SystemFailure : Error::PartialSend, error};
}

void Session::track(const INPUT& input) {
    if (input.type == INPUT_KEYBOARD) {
        const std::uint32_t id = keyIdentity(input.ki);
        if (input.ki.dwFlags & KEYEVENTF_KEYUP) {
            markReleased(id);
        } else {
            INPUT release = input;
            release.ki.dwFlags |= KEYEVENTF_KEYUP;
            release.ki.time = 0;
            markHeld(id, release);
        }
        return;
    }

    if (input.type == INPUT_MOUSE) {
        for (std::uint32_t i = 0; i < std::size(kMouseButtons); ++i) {
            const MouseButtonFlags& button = kMouseButtons[i];
            if (hasButton(input.mi, button.down, button.data)) {
                INPUT release{};
                release.type = INPUT_MOUSE;
                release.mi.dwFlags = button.up;
                release.mi.mouseData = button.data;
                release.mi.dwExtraInfo = input.mi.dwExtraInfo;
                markHeld(kMouse | i, release);
            }
            if (hasButton(input.mi, button.up, button.data)) markReleased(kMouse | i);
        }
    }
}

void Session::markHeld(std::uint32_t id, const INPUT& release) {
    if (!isHeldLocked(id)) held_.push_back({id, release});
}

void Session::markReleased(std::uint32_t id) {
    std::erase_if(held_, [id](const HeldInput& h) { return h.id == id; });
}

std::function<Status()> makeReleaser(Session& session, std::vector<INPUT> releases) {
    return [weak = session.weak_from_this(), releases = std::move(releases)]() -> Status {
        if (const auto alive = weak.lock()) return alive->release(releases);
        return {}; // the Input is gone and already released everything
    };
}

} // namespace inpututil::detail
