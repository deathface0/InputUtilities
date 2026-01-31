#pragma once

#include <vector>
#include <unordered_set>
#include "InputData.h"

class InputUtilitiesCore
{
public:
	InputUtilitiesCore(bool safemode);
	~InputUtilitiesCore();

	InputResult SetCursorPos(int x, int y, bool abs = true);
	InputResult SetCursorPos(int x, int y, int steps, int delay, bool abs = true);
	InputResult MouseEvent(DWORD dwFlags, DWORD mouseData = 0);
	InputResult ExtraClickDown(DWORD xbutton);
	InputResult ExtraClickUp(DWORD xbutton);
	InputResult MouseWheelRoll(int scrolls, MWheelDir dir, UINT delta = WHEEL_DELTA, MWheelAxis axis = Vertical);
	InputResult MouseWheelRoll(int scrolls, int delay, MWheelDir dir, UINT delta = WHEEL_DELTA, MWheelAxis axis = Vertical);
	
	InputResult vKeyDown(WORD vkCode);
	InputResult vKeyUp(WORD vkCode);
	InputResult unicodeKeyDown(wchar_t key);
	InputResult unicodeKeyUp(wchar_t key);
	InputResult scKeyDown(wchar_t key);
	InputResult scKeyUp(wchar_t key);
	InputResult keyDown(Event e);
	InputResult keyUp(Event e);

	InputResult vkMultiKeyDown(const std::vector<WORD>& vkCodes);
	InputResult vkMultiKeyUp(const std::vector<WORD>& vkCodes);
	InputResult unicodeMultiKeyDown(const std::vector<wchar_t>& keys);
	InputResult unicodeMultiKeyUp(const std::vector<wchar_t>& keys);
	InputResult scMultiKeyDown(const std::vector<wchar_t>& keys);
	InputResult scMultiKeyUp(const std::vector<wchar_t>& keys);
	InputResult multiKeyDown(const std::vector<Event>& keys);
	InputResult multiKeyUp(const std::vector<Event>& keys);

private:
	DWORD GetMouseID(DWORD flags, DWORD mouseData);
	DWORD GetMouseUpFlag(DWORD id, DWORD& mouseData);

	void reset();

private:
	std::unordered_set<Event, EventHash> runningInputs;
	bool safemode;
};