#pragma once
#include "base_game_event.h"
#include "matrix.h"

class CM4FireEvent : public CBaseGameEvent
{
public:
	CM4FireEvent(event_args_t *args);
	~CM4FireEvent() = default;

	void Execute(bool secondary);

private:
	void HandleShot();
	void HandleGrenadeLaunch();
	Vector GetShootDirection(const matrix3x3 &camera) const;
};