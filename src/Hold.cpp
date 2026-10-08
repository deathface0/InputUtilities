#include "inpututil/Hold.h"

#include <utility>

namespace inpututil {

Hold::Hold(std::function<Status()> release, Status status) noexcept
    : release_(std::move(release)), status_(status) {}

Hold::Hold(Hold&& other) noexcept
    : release_(std::exchange(other.release_, nullptr)), status_(other.status_) {}

Hold& Hold::operator=(Hold&& other) noexcept {
    if (this != &other) {
        release();
        release_ = std::exchange(other.release_, nullptr);
        status_ = other.status_;
    }
    return *this;
}

Hold::~Hold() { release(); }

Status Hold::release() {
    if (!release_) return {};
    const auto release = std::exchange(release_, nullptr);
    return release();
}

} // namespace inpututil
