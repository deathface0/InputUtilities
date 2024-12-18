#pragma once

#include "InputUtilitiesCore.h"

class InputUtilities : public InputUtilitiesCore
{
public:
	InputUtilities(bool safemode = false)
		: InputUtilitiesCore(safemode) {};

	bool leftClick(time_t pressed_ms = 0);
	bool rightClick(time_t pressed_ms = 0);
	bool middleClick(time_t pressed_ms = 0);
	bool extraClick(UINT button, time_t pressed_ms = 0);

	bool vKey(WORD vkCode, time_t pressed_ms = 0);
	bool unicodeKey(wchar_t key, time_t pressed_ms = 0);
	bool scKey(wchar_t key, time_t pressed_ms = 0);
	bool Key(Event e, time_t pressed_ms = 0);
	bool vkMultiKey(const std::vector<WORD>& vkCodes, time_t pressed_ms = 0);
	bool unicodeMultiKey(const std::vector<wchar_t>& keys, time_t pressed_ms = 0);
	bool scMultiKey(const std::vector<wchar_t>& keys, time_t pressed_ms = 0);

	bool typeStr(const std::wstring& str);
	bool scTypeStr(const std::wstring& str);
};