#pragma once

#include <Windows.h>
#include <stdexcept>
#include <vector>
#include <iostream>

#define UP    1
#define DOWN -1

#define MOUSE4 1
#define MOUSE5 2
#define MOUSE6 3
#define MOUSE7 4
#define MOUSE8 5

#define MOUSEEVENTF_LEFTDOWN    0x0002 /* left button down */
#define MOUSEEVENTF_LEFTUP      0x0004 /* left button up */
#define MOUSEEVENTF_RIGHTDOWN   0x0008 /* right button down */
#define MOUSEEVENTF_RIGHTUP     0x0010 /* right button up */
#define MOUSEEVENTF_MIDDLEDOWN  0x0020 /* middle button down */
#define MOUSEEVENTF_MIDDLEUP    0x0040 /* middle button up */

struct Event
{
	WORD mouse = -1;
	WORD vk = -1;
	char key = -1;
};

class InputUtilitiesCore
{
public:
	~InputUtilitiesCore();

	bool SetCursorPos(int x, int y);
	bool MouseEvent(WORD m_event);
	bool ExtraClickDown(int button);
	bool ExtraClickUp(int button);
	bool MouseWheelRoll(int scrolls, int delta);

	bool vkKeyDown(WORD vkCode);
	bool vkKeyUp(WORD vkCode);
	bool KeyDown(char key);
	bool KeyUp(char key);
	bool vkMultiKeyDown(const std::vector<WORD>& vkCodes);
	bool vkMultiKeyUp(const std::vector<WORD>& vkCodes);

	void reset();

private:
	std::vector<Event> runningInputs;
};

class InputUtilities : public InputUtilitiesCore
{
public:
	bool extraClick(int button);

	bool vkKey(WORD vkCode);
	bool directKey(char key);
	bool vkMultiKey(const std::vector<WORD>& vkCodes);
};