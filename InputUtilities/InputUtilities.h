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

	std::string get_utf8(const std::wstring& wstr)
	{
		if (wstr.empty()) return std::string();
		int sz = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), 0, 0, 0, 0);
		std::string res(sz, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &res[0], sz, 0, 0);
		return res;
	}

	std::wstring get_utf16(const std::string& str)
	{
		if (str.empty()) return std::wstring();
		int sz = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), 0, 0);
		std::wstring res(sz, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &res[0], sz);
		return res;
	}
};