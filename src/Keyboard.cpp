#include "inpututil/Keyboard.h"

#include <vector>

#include "detail/InputBuilders.h"
#include "detail/Session.h"

namespace inpututil {

namespace {

struct ComboEvents {
    std::vector<INPUT> downs; // in press order
    std::vector<INPUT> ups;   // in reverse order
};

// Events for pressing and releasing the combo; nullopt if any key is invalid.
std::optional<ComboEvents> buildCombo(const KeyCombo& combo, KeyMode mode, Backend& backend) {
    if (combo.keys.empty()) return std::nullopt;

    ComboEvents events;
    for (const Key& key : combo.keys) {
        const auto down = detail::makeKeyInput(key, mode, false, backend);
        const auto up = detail::makeKeyInput(key, mode, true, backend);
        if (!down || !up) return std::nullopt;
        events.downs.push_back(*down);
        events.ups.insert(events.ups.begin(), *up);
    }
    return events;
}

} // namespace

Status Keyboard::down(Key key) {
    if (!session_) return Error::InvalidArgument;
    const auto input = detail::makeKeyInput(key, mode_, false, session_->backend());
    if (!input) return Error::InvalidArgument;
    return session_->send(std::span(&*input, 1));
}

Status Keyboard::up(Key key) {
    if (!session_) return Error::InvalidArgument;
    const auto input = detail::makeKeyInput(key, mode_, true, session_->backend());
    if (!input) return Error::InvalidArgument;
    return session_->send(std::span(&*input, 1), detail::SendMode::BestEffort);
}

Status Keyboard::tap(Key key, std::chrono::milliseconds hold) { return press(KeyCombo{{key}}, hold); }

Status Keyboard::press(const KeyCombo& combo, std::chrono::milliseconds hold) {
    if (!session_) return Error::InvalidArgument;
    const auto events = buildCombo(combo, mode_, session_->backend());
    if (!events) return Error::InvalidArgument;

    if (hold <= std::chrono::milliseconds::zero()) {
        std::vector<INPUT> batch = events->downs;
        batch.insert(batch.end(), events->ups.begin(), events->ups.end());
        const Status status = session_->send(batch);
        if (!status) session_->send(events->ups, detail::SendMode::BestEffort); // never leave keys down
        return status;
    }

    if (const Status status = session_->send(events->downs); !status) {
        session_->send(events->ups, detail::SendMode::BestEffort);
        return status;
    }
    session_->wait(hold);
    return session_->send(events->ups, detail::SendMode::BestEffort);
}

Status Keyboard::press(std::string_view combo, std::chrono::milliseconds hold) {
    const auto parsed = KeyCombo::parse(combo);
    if (!parsed) return Error::InvalidArgument;
    return press(*parsed, hold);
}

Hold Keyboard::hold(Key key) { return hold(KeyCombo{{key}}); }

Hold Keyboard::hold(const KeyCombo& combo) {
    if (!session_) return Hold(nullptr, Error::InvalidArgument);
    auto events = buildCombo(combo, mode_, session_->backend());
    if (!events) return Hold(nullptr, Error::InvalidArgument);

    if (const Status status = session_->send(events->downs); !status) {
        session_->send(events->ups, detail::SendMode::BestEffort);
        return Hold(nullptr, status);
    }

    auto release = [session = session_->weak_from_this(), ups = std::move(events->ups)]() -> Status {
        if (const auto alive = session.lock()) return alive->send(ups, detail::SendMode::BestEffort);
        return {}; // the Input is gone and already released everything
    };
    return Hold(std::move(release), Status{});
}

Hold Keyboard::hold(std::string_view combo) {
    const auto parsed = KeyCombo::parse(combo);
    if (!parsed) return Hold(nullptr, Error::InvalidArgument);
    return hold(*parsed);
}

bool Keyboard::isHeld(Key key) const {
    if (!session_) return false;
    const auto input = detail::makeKeyInput(key, mode_, false, session_->backend());
    return input && session_->isHeld(*input);
}

} // namespace inpututil
