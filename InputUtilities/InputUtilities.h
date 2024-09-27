#pragma once

#include "InputUtilitiesCore.h"

class InputUtilities : public InputUtilitiesCore
{
public:
	bool extraClick(int button);

	bool vkKey(WORD vkCode);
	bool directKey(char key);
	bool vkMultiKey(const std::vector<WORD>& vkCodes);
};