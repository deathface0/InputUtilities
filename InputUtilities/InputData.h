#pragma once

#include <Windows.h>
#include <cstdint>

#define EXBUTTON(n) EXBUTTON##n = n /* mouse extra button n */

enum InputResult : DWORD {
	Success = 0x00,
	SystemFailure,
	MouseSamePos,
	InvalidMapping,
	InvalidType,
	UndefinedError
};

enum class InputType : uint8_t {
	VK = 0,
	SC = 1,
	UC = 2,
	Mouse = 3
};

struct Event {
	InputType type;
	DWORD code;

	bool operator==(const Event& other) const {
		return type == other.type && code == other.code;
	}
};

struct EventHash {
	std::size_t operator()(const Event& k) const {
		return (static_cast<uint64_t>(k.type) << 32) | k.code;
	}
};


enum MWheelAxis {
	Vertical = 0x0800,
	Horizontal = 0x01000
};

enum MWheelDir : int {
	Up = 1,
	Down = -1,
	Right = 1,
	Left = -1
};


