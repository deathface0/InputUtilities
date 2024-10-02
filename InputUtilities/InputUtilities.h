#pragma once

#include "InputUtilitiesCore.h"

class InputUtilities : public InputUtilitiesCore
{
public:
	bool leftClick(time_t pressed_ms = 0);
	bool rightClick(time_t pressed_ms = 0);
	bool middleClick(time_t pressed_ms = 0);
	bool extraClick(UINT button, time_t pressed_ms = 0);

	bool vkKey(DWORD vkCode, time_t pressed_ms = 0);
	bool mappedKey(char key, time_t pressed_ms = 0);
	bool Key(DWORD vkCode, time_t pressed_ms = 0);
	bool vkMultiKey(const std::vector<DWORD>& vkCodes, time_t pressed_ms = 0);
	bool mappedMultiKey(const std::vector<char>& keys, time_t pressed_ms = 0);
	bool MultiKey(const std::vector<DWORD>& vkCodes, time_t pressed_ms = 0);

	bool typeStr(const std::string& str);
};