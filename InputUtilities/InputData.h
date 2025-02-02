#pragma once

#include <Windows.h>
#include <string>

enum IUerrorCode : DWORD {
	SUCCESS = 0x0,
	UNDEFINED_INPUT_TYPE = 0x28A,
	MOUSE_SAME_POSITON
};

struct Result {
	DWORD errorcode;

	Result(DWORD errorcode = GetLastError())
		:errorcode(errorcode) {}
};

#define EXBUTTON(n) EXBUTTON##n = n /* mouse extra button n */

enum MouseAction {
	LEFTDOWN = 0x0002,
	LEFTUP = 0x0004,
	RIGHTDOWN = 0x0008,
	RIGHTUP = 0x0010,
	MIDDLEDOWN = 0x0020,
	MIDDLEUP = 0x0040
};

enum MWheelAxis {
	VERTICAL = 0x0800,
	HORIZONTAL = 0x01000
};

enum MWheelDir {
	UP = 1,
	DOWN = -1,
	RIGHT = 1,
	LEFT = -1
};

enum InputType {
	MOUSE = 0x01,
	UC,
	VK,
	SC
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

	bool operator==(const Key& other) const {
		return isVK == other.isVK && (isVK ? vkKey == other.vkKey : charKey == other.charKey);
	}
};

struct Event {
	InputType type;
	Key iu_event;

	Event(InputType type, Key iu_event)
		: type(type), iu_event(iu_event) {}

	bool operator==(const Event& other) const {
		return type == other.type && iu_event == other.iu_event;
	}
};

namespace std {
	template<>
	struct hash<Key> {
		size_t operator()(const Key& k) const {
			return hash<bool>()(k.isVK) ^ (k.isVK ? hash<WORD>()(k.vkKey) : hash<wchar_t>()(k.charKey));
		}
	};

	template<>
	struct hash<Event> {
		size_t operator()(const Event& e) const {
			return hash<int>()(static_cast<int>(e.type)) ^ hash<Key>()(e.iu_event);
		}
	};
}