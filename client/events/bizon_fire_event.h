#pragma once
#include "base_game_event.h"
#include "matrix.h"

class CBizonFireEvent : public CBaseGameEvent
{
public:
	explicit CBizonFireEvent(event_args_t *args);
	void Execute();
private:
	Vector GetShootDirection(const matrix3x3 &camera) const;
};
