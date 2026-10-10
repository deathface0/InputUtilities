#include "inpututil/Input.h"

#include <vector>

#include "detail/Coords.h"
#include "detail/InputBuilders.h"
#include "detail/Path.h"
#include "detail/Session.h"
#include "detail/Text.h"

namespace inpututil {

namespace steps = detail::steps;

namespace {

template <typename... Fs> struct Overloaded : Fs... {
    using Fs::operator()...;
};

// Checks a step without sending anything, so a bad step anywhere fails the
// whole sequence up front.
Status validate(const detail::SequenceStep& step, KeyMode mode, Backend& backend) {
    const auto valid = [](bool ok) -> Status { return ok ? Status{} : Error::InvalidArgument; };
    const auto check = Overloaded{
        [&](const steps::Invalid&) { return valid(false); },
        [&](const steps::KeyDown& s) {
            return valid(detail::makeKeyInput(s.key, mode, false, backend).has_value());
        },
        [&](const steps::KeyUp& s) {
            return valid(detail::makeKeyInput(s.key, mode, true, backend).has_value());
        },
        [&](const steps::Press& s) {
            return valid(detail::makeComboEvents(s.combo.keys, mode, backend).has_value());
        },
        [&](const steps::Type& s) {
            std::vector<detail::InputGroup> groups;
            return detail::buildTextGroups(s.text, s.options.mode, s.options.fallbackToUnicode, mode, backend,
                                           groups);
        },
        [&](const steps::Click& s) { return valid(s.options.count >= 1); },
        [&](const steps::Scroll& s) { return valid(detail::wheelDelta(s.notches).has_value()); },
        [](const auto&) { return Status{}; }, // moves and waits are always valid
    };
    return std::visit(check, step);
}

} // namespace

Status Input::play(const Sequence& sequence) {
    if (!session_) return Error::InvalidArgument;
    Backend& backend = session_->backend();
    const KeyMode mode = keyboard.mode();

    for (const auto& step : sequence.steps_)
        if (const Status status = validate(step, mode, backend); !status) return status;

    // A held abort key stops the sequence before it starts.
    if (!sequence.empty())
        if (const Status status = session_->checkAbort(); !status) return status;

    std::vector<INPUT> pending;       // instant events waiting to go in one batch
    std::vector<INPUT> releases;      // release of everything this sequence pressed, last first
    std::optional<Point> pendingGoal; // where the pending batch leaves the cursor, if known

    const auto queueKey = [&](const Key& key, bool down) {
        pending.push_back(*detail::makeKeyInput(key, mode, !down, backend));
        if (down) releases.insert(releases.begin(), *detail::makeKeyInput(key, mode, true, backend));
    };
    const auto queueButton = [&](MouseButton button, bool down) {
        pending.push_back(detail::makeButtonInput(button, !down));
        if (down) releases.insert(releases.begin(), detail::makeButtonInput(button, true));
    };
    const auto flush = [&]() -> Status {
        if (pending.empty()) return {};
        // A batch that moves the cursor waits for Windows to apply it, so later steps read the real position.
        const std::optional<Point> before = pendingGoal ? backend.cursorPos() : std::nullopt;
        const Status status = session_->send(pending);
        if (status && pendingGoal && before) detail::settleCursor(backend, *pendingGoal, *before);
        pending.clear();
        pendingGoal.reset();
        return status;
    };
    // On failure, release what this sequence pressed and is still down (never the user's own holds).
    const auto fail = [&](Status status) {
        session_->release(releases);
        return status;
    };

    const auto zero = std::chrono::milliseconds::zero();
    // Instant steps join the pending batch; the others flush it first and run
    // through the public Keyboard/Mouse methods.
    const auto run = Overloaded{
        [](const steps::Invalid&) -> Status { return Error::InvalidArgument; }, // rejected by validate()
        [&](const steps::KeyDown& s) -> Status {
            queueKey(s.key, true);
            return {};
        },
        [&](const steps::KeyUp& s) -> Status {
            queueKey(s.key, false);
            return {};
        },
        [&](const steps::Press& s) -> Status {
            if (s.hold > zero) {
                if (const Status status = flush(); !status) return status;
                return keyboard.press(s.combo, s.hold);
            }
            for (const Key& key : s.combo.keys) queueKey(key, true);
            for (auto it = s.combo.keys.rbegin(); it != s.combo.keys.rend(); ++it) queueKey(*it, false);
            return {};
        },
        [&](const steps::Type& s) -> Status {
            if (const Status status = flush(); !status) return status;
            return keyboard.type(std::wstring_view(s.text), s.options);
        },
        [&](const steps::ButtonDown& s) -> Status {
            queueButton(s.button, true);
            return {};
        },
        [&](const steps::ButtonUp& s) -> Status {
            queueButton(s.button, false);
            return {};
        },
        [&](const steps::Click& s) -> Status {
            if (s.options.hold > zero || s.options.interval > zero) {
                if (const Status status = flush(); !status) return status;
                return mouse.click(s.button, s.options);
            }
            for (int i = 0; i < s.options.count; ++i) {
                queueButton(s.button, true);
                queueButton(s.button, false);
            }
            return {};
        },
        [&](const steps::MoveTo& s) -> Status {
            if (s.motion.duration > zero || mouse.verifyCursor_) {
                if (const Status status = flush(); !status) return status;
                return mouse.moveTo(s.target, s.motion);
            }
            const Rect screen = backend.virtualScreen();
            pending.push_back(detail::makeAbsoluteMove(s.target, screen));
            pendingGoal = detail::clampToScreen(s.target, screen);
            return {};
        },
        [&](const steps::MoveBy& s) -> Status {
            // Needs the real position after everything before it.
            if (const Status status = flush(); !status) return status;
            return mouse.moveBy(s.dx, s.dy, s.motion);
        },
        [&](const steps::MoveRaw& s) -> Status {
            if (s.motion.duration > zero) {
                if (const Status status = flush(); !status) return status;
                return mouse.moveRaw(s.dx, s.dy, s.motion);
            }
            if (s.dx != 0 || s.dy != 0) {
                pending.push_back(detail::makeRelativeMove(s.dx, s.dy));
                pendingGoal.reset(); // the final position in pixels is unknown
            }
            return {};
        },
        [&](const steps::Scroll& s) -> Status {
            if (s.duration > zero) {
                if (const Status status = flush(); !status) return status;
                return s.horizontal ? mouse.scrollHorizontal(s.notches, s.duration)
                                    : mouse.scroll(s.notches, s.duration);
            }
            if (const int delta = *detail::wheelDelta(s.notches); delta != 0)
                pending.push_back(detail::makeWheelInput(delta, s.horizontal));
            return {};
        },
        [&](const steps::Wait& s) -> Status {
            if (const Status status = flush(); !status) return status;
            return session_->wait(s.duration);
        },
    };

    for (const auto& step : sequence.steps_)
        if (const Status status = std::visit(run, step); !status) return fail(status);

    if (const Status status = flush(); !status) return fail(status);
    return {};
}

} // namespace inpututil
