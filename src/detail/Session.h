#pragma once

#include <windows.h>

#include <chrono>
#include <cstdint>
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
class Session {
public:
    Session(std::shared_ptr<Backend> backend, std::uintptr_t extraInfoTag);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    Backend& backend() noexcept { return *backend_; }

    /// Injects the batch with a single SendInput call and tracks the events
    /// the system accepted. Events with dwExtraInfo == 0 get the session tag.
    Status send(std::span<const INPUT> inputs, SendMode mode = SendMode::StopOnFailure);

    /// Releases every held key and button, last pressed first. Whatever could
    /// not be released stays tracked so a later call can retry it.
    Status releaseAll();

    /// Number of keys and buttons currently held down.
    std::size_t heldCount() const;

    /// Waits through the backend clock.
    Status wait(std::chrono::nanoseconds duration);

private:
    struct HeldInput {
        std::uint32_t id;
        INPUT release; // the event that releases it
    };

    Status sendLocked(std::span<const INPUT> inputs, SendMode mode);
    void track(const INPUT& input);
    void hold(std::uint32_t id, const INPUT& release);
    void unhold(std::uint32_t id);

    std::shared_ptr<Backend> backend_;
    std::uintptr_t extraInfoTag_;

    mutable std::timed_mutex mutex_;
    std::vector<HeldInput> held_; // in press order
};

} // namespace inpututil::detail
