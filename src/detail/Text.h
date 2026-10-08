#pragma once

#include <windows.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "inpututil/Backend.h"
#include "inpututil/Keyboard.h"
#include "inpututil/Status.h"

namespace inpututil::detail {

/// UTF-8 to UTF-16; nullopt if the input is not valid UTF-8.
std::optional<std::wstring> utf8ToUtf16(std::string_view utf8);

/// Events that type one character (with its modifiers), sent as one batch.
using InputGroup = std::vector<INPUT>;

/// Builds one group per typed character. Line breaks become Enter and tabs
/// Tab. Fails before anything is sent: InvalidArgument for unpaired UTF-16
/// surrogates, UnmappableCharacter for a character without a key when
/// Keystrokes mode may not fall back to Unicode.
Status buildTextGroups(std::wstring_view text, TextMode mode, bool fallbackToUnicode, KeyMode keyMode,
                       Backend& backend, std::vector<InputGroup>& groups);

} // namespace inpututil::detail
