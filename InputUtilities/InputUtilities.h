#pragma once

#include "InputUtilitiesCore.h"

class InputUtilities : public InputUtilitiesCore
{
public:
	InputUtilities(bool safemode = false)
		: InputUtilitiesCore(safemode) {};

	Result leftClick(time_t pressed_ms = 0);
	Result rightClick(time_t pressed_ms = 0);
	Result middleClick(time_t pressed_ms = 0);
	Result extraClick(UINT button, time_t pressed_ms = 0);

	Result vKey(WORD vkCode, time_t pressed_ms = 0);
	Result unicodeKey(wchar_t key, time_t pressed_ms = 0);
	Result scKey(wchar_t key, time_t pressed_ms = 0);
	Result Key(Event e, time_t pressed_ms = 0);
	Result vkMultiKey(const std::vector<WORD>& vkCodes, time_t pressed_ms = 0);
	Result unicodeMultiKey(const std::vector<wchar_t>& keys, time_t pressed_ms = 0);
	Result scMultiKey(const std::vector<struct Key>& keys, time_t pressed_ms = 0);

	Result typeStr(const std::wstring& str);
	Result scTypeStr(const std::wstring& str);
};