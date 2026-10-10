#include "inpututil/Status.h"

#include <windows.h>

namespace inpututil {

std::string_view errorName(Error error) noexcept {
    switch (error) {
    case Error::None: return "None";
    case Error::SystemFailure: return "SystemFailure";
    case Error::PartialSend: return "PartialSend";
    case Error::InvalidArgument: return "InvalidArgument";
    case Error::UnmappableCharacter: return "UnmappableCharacter";
    case Error::TargetNotReached: return "TargetNotReached";
    case Error::Aborted: return "Aborted";
    }
    return "Unknown";
}

namespace {

std::string systemMessage(std::uint32_t code) {
    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                            FORMAT_MESSAGE_IGNORE_INSERTS,
                                        nullptr, code, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    if (length == 0 || buffer == nullptr) return {};

    std::wstring wide(buffer, length);
    LocalFree(buffer);
    while (!wide.empty() && (wide.back() == L'\r' || wide.back() == L'\n' || wide.back() == L' '))
        wide.pop_back();

    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0,
                                         nullptr, nullptr);
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), utf8.data(), size, nullptr,
                        nullptr);
    return utf8;
}

} // namespace

std::string Status::message() const {
    std::string text(errorName(error_));
    if (win32Error_ != 0) {
        text += " (Win32 error " + std::to_string(win32Error_);
        if (const std::string sys = systemMessage(win32Error_); !sys.empty()) text += ": " + sys;
        text += ")";
    }
    return text;
}

} // namespace inpututil
