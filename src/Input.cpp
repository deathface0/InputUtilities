#include "inpututil/Input.h"

#include "detail/Session.h"

namespace inpututil {

Input::Input(Config config)
    : session_(std::make_shared<detail::Session>(config.backend ? std::move(config.backend) : win32Backend(),
                                                 config.extraInfoTag)),
      releaseOnDestroy_(config.releaseOnDestroy), keyboard(session_.get(), config.keyMode) {}

Input::~Input() {
    if (session_ && releaseOnDestroy_) session_->releaseAll();
}

Input::Input(Input&& other) noexcept
    : session_(std::move(other.session_)), releaseOnDestroy_(other.releaseOnDestroy_),
      keyboard(session_.get(), other.keyboard.mode_) {
    other.keyboard.session_ = nullptr;
}

Input& Input::operator=(Input&& other) noexcept {
    if (this != &other) {
        if (session_ && releaseOnDestroy_) session_->releaseAll();
        session_ = std::move(other.session_);
        releaseOnDestroy_ = other.releaseOnDestroy_;
        keyboard.session_ = session_.get();
        keyboard.mode_ = other.keyboard.mode_;
        other.keyboard.session_ = nullptr;
    }
    return *this;
}

Status Input::releaseAll() {
    if (!session_) return Error::InvalidArgument;
    return session_->releaseAll();
}

std::size_t Input::heldCount() const { return session_ ? session_->heldCount() : 0; }

} // namespace inpututil
