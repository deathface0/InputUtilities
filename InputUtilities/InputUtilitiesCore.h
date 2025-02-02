#pragma once

#include <Windows.h>
#include <vector>
#include <unordered_set>
#include "InputData.h"

class InputUtilitiesCore
{
public:
	InputUtilitiesCore(bool safemode);
	~InputUtilitiesCore();

	Result SetCursorPos(int x, int y, bool abs = true);
	Result SetCursorPos(int x, int y, int steps, int delay, bool abs = true);
	Result MouseEvent(WORD m_event);
	Result ExtraClickDown(WORD xbutton);
	Result ExtraClickUp(WORD xbutton);
	Result MouseWheelRoll(int scrolls, MWheelDir dir, UINT delta = WHEEL_DELTA, MWheelAxis axis = VERTICAL);
	Result MouseWheelRoll(int scrolls, int delay, MWheelDir dir, UINT delta = WHEEL_DELTA, MWheelAxis axis = VERTICAL);
	
	Result vKeyDown(WORD vkCode);
	Result vKeyUp(WORD vkCode);
	Result unicodeKeyDown(wchar_t key);
	Result unicodeKeyUp(wchar_t key);
	Result scKeyDown(wchar_t key);
	Result scKeyUp(wchar_t key);
	Result keyDown(Event e);
	Result keyUp(Event e);
	Result vkMultiKeyDown(const std::vector<WORD>& vkCodes);
	Result vkMultiKeyUp(const std::vector<WORD>& vkCodes);
	Result unicodeMultiKeyDown(const std::vector<wchar_t>& keys);
	Result unicodeMultiKeyUp(const std::vector<wchar_t>& keys);
	Result scMultiKeyDown(const std::vector<Key>& keys);
	Result scMultiKeyUp(const std::vector<Key>& keys);
	Result multiKeyDown(const std::vector<Event>& keys);
	Result multiKeyUp(const std::vector<Event>& keys);

	std::string get_utf8(const std::wstring& wstr);
	std::wstring get_utf16(const std::string& str);

private:
	bool isExtraMouseButton(DWORD m_event);
	bool isButtonUp(DWORD button);

	void reset();

private:
	std::unordered_set<Event> runningInputs;
	bool safemode;
};