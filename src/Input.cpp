#include "inpututil/Input.h"

#include "detail/InputBuilders.h"
#include "detail/Session.h"

namespace inpututil {

Input::Input(Config config)
    : session_(std::make_shared<detail::Session>(config.backend ? std::move(config.backend) : win32Backend(),
                                                 config.extraInfoTag)),
      releaseOnDestroy_(config.releaseOnDestroy), keyboard(session_.get(), config.keyMode),
      mouse(session_.get(), config.verifyCursor) {
    setAbortKey(config.abortKey);
}

namespace {

// Releases from a destructor or a noexcept assignment, where an exception
// (std::bad_alloc, a mutex error) would terminate the program.
void releaseQuietly(detail::Session& session) noexcept {
    try {
        session.releaseAll();
    } catch (...) {
    }
}

} // namespace

Input::~Input() {
    if (session_ && releaseOnDestroy_) releaseQuietly(*session_);
}

// Keyboard and Mouse keep pointing at the same Session, now owned by this Input.
Input::Input(Input&&) noexcept = default;

Input& Input::operator=(Input&& other) noexcept {
    if (this != &other) {
        if (session_ && releaseOnDestroy_) releaseQuietly(*session_);
        session_ = std::move(other.session_);
        releaseOnDestroy_ = other.releaseOnDestroy_;
        keyboard = std::move(other.keyboard);
        mouse = std::move(other.mouse);
    }
    return *this;
}

Status Input::releaseAll() {
    if (!session_) return Error::InvalidArgument;
    return session_->releaseAll();
}

Status Input::sendRaw(std::span<const tagINPUT> events) {
    if (!session_) return Error::InvalidArgument;
    if (events.empty()) return {};
    return session_->send(events);
}

std::size_t Input::heldCount() const { return session_ ? session_->heldCount() : 0; }

void Input::requestAbort() {
    if (session_) session_->requestAbort();
}

void Input::clearAbortRequest() {
    if (session_) session_->clearAbortRequest();
}

bool Input::abortRequested() const { return session_ && session_->abortRequested(); }

void Input::setAbortKey(std::optional<Key> key) {
    if (!session_) return;
    const auto resolved = key ? detail::resolveKey(*key, session_->backend()) : std::nullopt;
    session_->setAbortVk(resolved ? resolved->vk : 0);
}

} // namespace inpututil
