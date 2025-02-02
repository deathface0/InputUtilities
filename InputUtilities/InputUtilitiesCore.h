#pragma once

#include <Windows.h>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <iostream>

#define UP						1
#define DOWN					-1

#define XBUTTON1				0x0001 /* mouse extra button 1 */
#define XBUTTON2				0x0002 /* mouse extra button 2 */
#define XBUTTON3				0x0003 /* mouse extra button 3 */
#define XBUTTON4				0x0004 /* mouse extra button 4 */

#define MOUSE_LEFTDOWN    0x0002 /* left button down */
#define MOUSE_LEFTUP      0x0004 /* left button up */
#define MOUSE_RIGHTDOWN   0x0008 /* right button down */
#define MOUSE_RIGHTUP     0x0010 /* right button up */
#define MOUSE_MIDDLEDOWN  0x0020 /* middle button down */
#define MOUSE_MIDDLEUP    0x0040 /* middle button up */

enum IU_TYPE {
	IU_MOUSE = 0x01, IU_UC, IU_VK, IU_SCK
};

struct Key {
	bool isVK;
	union {
		wchar_t charKey;
		WORD vkKey;
	};

	Key(wchar_t ch) : isVK(false), charKey(ch) {}
	Key(char ch) : isVK(false), charKey(static_cast<wchar_t>(ch)) {}
	Key(int vk) : isVK(true), vkKey(vk) {}
};

struct Event {
	IU_TYPE type;
	WORD iu_event;

	Event(IU_TYPE type, WORD iu_event)
		:type(type), iu_event(iu_event) {}

	bool operator==(const Event& other) const {
		return type == other.type && iu_event == other.iu_event;
	}
};

namespace std {
	template<>
	struct hash<Event> {
		size_t operator()(const Event& e) const {
			return hash<int>()(static_cast<int>(e.type)) ^ hash<WORD>()(e.iu_event);
		}
	};
}

class InputUtilitiesCore
{
public:
	InputUtilitiesCore(bool safemode);
	~InputUtilitiesCore();

	bool SetCursorPos(int x, int y);
	bool MouseEvent(WORD m_event);
	bool ExtraClickDown(WORD xbutton);
	bool ExtraClickUp(WORD xbutton);
	bool MouseWheelRoll(int scrolls, int delta);

	bool vKeyDown(WORD vkCode);
	bool vKeyUp(WORD vkCode);
	bool unicodeKeyDown(wchar_t key);
	bool unicodeKeyUp(wchar_t key);
	bool scKeyDown(wchar_t key);
	bool scKeyUp(wchar_t key);
	bool keyDown(Event e);
	bool keyUp(Event e);
	bool vkMultiKeyDown(const std::vector<WORD>& vkCodes);
	bool vkMultiKeyUp(const std::vector<WORD>& vkCodes);
	bool unicodeMultiKeyDown(const std::vector<wchar_t>& keys);
	bool unicodeMultiKeyUp(const std::vector<wchar_t>& keys);
	bool scMultiKeyDown(const std::vector<Key>& keys);
	bool scMultiKeyUp(const std::vector<Key>& keys);

	std::string get_utf8(const std::wstring& wstr);
	std::wstring get_utf16(const std::string& str);

private:
	bool isExtraMouseButton(DWORD m_event);
	bool isButtonUp(DWORD button);

	bool removeEvent(const Event& e);
	void reset();

private:
	std::unordered_set<Event> runningInputs;
	bool safemode;
};