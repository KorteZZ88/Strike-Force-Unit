#pragma once

// entity_state_t.iuser4 is transmitted as 8 bits in delta.lst.
constexpr int STICK_CAMERA_MARKER = 0xC7;
constexpr float STICK_CAMERA_TURN_LIMIT = 90.0f;
static_assert(STICK_CAMERA_MARKER > 0 && STICK_CAMERA_MARKER <= 0xFF,
	"Stick camera marker must fit the network field");
