#pragma once
#include "base_game_event.h"
#include "matrix.h"

class CP90FireEvent : public CBaseGameEvent
{
public:
	explicit CP90FireEvent(event_args_t *args);
	void Execute();
private:
	Vector GetShootDirection(const matrix3x3 &camera) const;
};
