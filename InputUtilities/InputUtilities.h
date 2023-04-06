#pragma once

#include <Windows.h>
#include <stdexcept>
#include <vector>

#define UP    1
#define DOWN -1

class MouseUtilities
{
private:
public:
	static bool move(int x, int y);
	static bool rightClick();
	static bool leftClick();
	static bool wheelClick();
	static bool extraClick(int button);
	static bool wheelRoll(int scrolls, int delta);
};

class KeyboardUtilities
{
private:
public:
	static bool virtualInput(WORD vkCode);
	static bool directInput(char key);
	static bool virtualCombinedInput(const std::vector<WORD>& vkCodes);
	static void typeStr(const char* str);
	static void typeStr(char* str);
};