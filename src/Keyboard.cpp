#include "inpututil/Keyboard.h"

#include <optional>
#include <random>
#include <vector>

#include "detail/InputBuilders.h"
#include "detail/Session.h"
#include "detail/Text.h"

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
    if (const Status status = session_->wait(hold); !status) return status; // aborted: already released
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

Status Keyboard::type(std::string_view utf8, const TypeOptions& options) {
    if (!session_) return Error::InvalidArgument;
    const auto text = detail::utf8ToUtf16(utf8);
    if (!text) return Error::InvalidArgument;
    return type(std::wstring_view(*text), options);
}

Status Keyboard::type(std::u8string_view text, const TypeOptions& options) {
    return type(std::string_view(reinterpret_cast<const char*>(text.data()), text.size()), options);
}

Status Keyboard::type(std::wstring_view text, const TypeOptions& options) {
    if (!session_) return Error::InvalidArgument;

    std::vector<detail::InputGroup> groups;
    if (const Status status = detail::buildTextGroups(text, options.mode, options.fallbackToUnicode, mode_,
                                                      session_->backend(), groups);
        !status)
        return status;

    std::optional<std::mt19937> rng;
    if (options.jitter > std::chrono::milliseconds::zero())
        rng.emplace(options.seed != 0 ? options.seed : std::random_device{}());

    for (std::size_t i = 0; i < groups.size(); ++i) {
        // The first character only checks the abort key; the others wait delay ± jitter first.
        auto pause = std::chrono::milliseconds::zero();
        if (i > 0) {
            pause = options.delay;
            if (rng) {
                const auto jitter = options.jitter.count();
                pause += std::chrono::milliseconds(std::uniform_int_distribution<long long>(-jitter, jitter)(*rng));
            }
        }
        if (const Status status = session_->wait(pause); !status) return status;

        if (const Status status = session_->send(groups[i]); !status) {
            // Release whatever this character pressed (its modifiers included).
            std::vector<INPUT> ups;
            for (const INPUT& event : groups[i]) {
                if (event.ki.dwFlags & KEYEVENTF_KEYUP) continue;
                INPUT up = event;
                up.ki.dwFlags |= KEYEVENTF_KEYUP;
                ups.insert(ups.begin(), up);
            }
            session_->send(ups, detail::SendMode::BestEffort);
            return status;
        }
    }
    return {};
}

bool Keyboard::isHeld(Key key) const {
    if (!session_) return false;
    const auto input = detail::makeKeyInput(key, mode_, false, session_->backend());
    return input && session_->isHeld(*input);
}

} // namespace inpututil
