#pragma once

#include <Windows.h>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>

#define UP						1
#define DOWN					-1

#define XBUTTON1				0x0001 /* mouse extra button 1 */
#define XBUTTON2				0x0002 /* mouse extra button 2 */

#define MOUSEEVENTF_LEFTDOWN    0x0002 /* left button down */
#define MOUSEEVENTF_LEFTUP      0x0004 /* left button up */
#define MOUSEEVENTF_RIGHTDOWN   0x0008 /* right button down */
#define MOUSEEVENTF_RIGHTUP     0x0010 /* right button up */
#define MOUSEEVENTF_MIDDLEDOWN  0x0020 /* middle button down */
#define MOUSEEVENTF_MIDDLEUP    0x0040 /* middle button up */

struct Event
{
	DWORD mouse;
	char vk;
	char key;

	Event(DWORD mouse, char vk, char key) : mouse(mouse), vk(vk), key(key) {}

	friend std::ostream& operator<<(std::ostream& os, const Event& e);
};

class InputUtilitiesCore
{
public:
	~InputUtilitiesCore();

	bool SetCursorPos(int x, int y);
	bool MouseEvent(DWORD m_event);
	bool ExtraClickDown(DWORD xbutton);
	bool ExtraClickUp(DWORD xbutton);
	bool MouseWheelRoll(int scrolls, int delta);

	bool vkKeyDown(char vkCode);
	bool vkKeyUp(char vkCode);
	bool KeyDown(char key);
	bool KeyUp(char key);
	bool vkMultiKeyDown(const std::vector<char>& vkCodes);
	bool vkMultiKeyUp(const std::vector<char>& vkCodes);

private:
	bool isUppercaseOn();
	bool isExtraMouseButton(DWORD m_event);
	bool isButtonUp(DWORD button);
	bool removeEvent(const Event& e);
	void reset();

private:
	std::vector<Event> runningInputs;
};