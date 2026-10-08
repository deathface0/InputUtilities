#pragma once

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

#include "inpututil/Backend.h"
#include "inpututil/Status.h"

namespace inpututil::detail {

enum class SendMode {
    StopOnFailure, ///< Report a partial batch as an error and stop.
    BestEffort,    ///< Retry the rejected events one by one (used when releasing).
};

/// Single path through which every injected event goes. Sends batches with
/// one SendInput call, tracks what is held down (from the INPUT events
/// themselves, whoever built them) and releases it in reverse order.
/// Thread-safe: sending and tracking happen under the same lock.
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(std::shared_ptr<Backend> backend, std::uintptr_t extraInfoTag);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    Backend& backend() noexcept { return *backend_; }

    /// Injects the batch with a single SendInput call and tracks the events
    /// the system accepted. Events with dwExtraInfo == 0 get the session tag.
    Status send(std::span<const INPUT> inputs, SendMode mode = SendMode::StopOnFailure);

    /// Releases every held key and button, last pressed first. Whatever could
    /// not be released stays tracked so a later call can retry it.
    Status releaseAll();

    /// releaseAll() for a dying process: never throws and never blocks for
    /// long. If the lock cannot be taken (the crash happened while sending),
    /// the releases are sent anyway without touching the tracking.
    void emergencyRelease() noexcept;

    /// Number of keys and buttons currently held down.
    std::size_t heldCount() const;

    /// Whether the key or button pressed by this event is currently held.
    bool isHeld(const INPUT& event) const;

    /// Virtual key that aborts long operations (0 = none). Safe to call from any thread.
    void setAbortVk(std::uint16_t vk) noexcept { abortVk_.store(vk); }

    /// If the abort key is down: releases everything and returns Aborted.
    Status checkAbort();

    /// Waits until the deadline on the backend clock (returns at once if it
    /// passed). The single waiting point of the library: with an abort key it
    /// polls the key every 10 ms and returns Aborted when it goes down.
    Status waitUntil(Backend::Clock::time_point deadline);

    /// Waits for the duration on the backend clock. A zero duration still
    /// checks the abort key.
    Status wait(std::chrono::nanoseconds duration);

private:
    struct HeldInput {
        std::uint32_t id;
        INPUT release; // the event that releases it
    };

    Status sendLocked(std::span<const INPUT> inputs, SendMode mode);
    std::vector<INPUT> releasesLocked() const; // release events, last pressed first
    void track(const INPUT& input);
    void markHeld(std::uint32_t id, const INPUT& release);
    void markReleased(std::uint32_t id);

    std::shared_ptr<Backend> backend_;
    std::uintptr_t extraInfoTag_;

    std::atomic<std::uint16_t> abortVk_{0};

    mutable std::timed_mutex mutex_;
    std::vector<HeldInput> held_; // in press order
};

/// Callback for a Hold: sends the releases (best effort) if the session is
/// still alive; otherwise succeeds, as the session released everything when it died.
std::function<Status()> makeReleaser(Session& session, std::vector<INPUT> releases);

/// Process-wide list of live sessions, used by the emergency release
/// (defined in EmergencyRelease.cpp).
void registerSession(Session* session);
void unregisterSession(Session* session);

} // namespace inpututil::detail
