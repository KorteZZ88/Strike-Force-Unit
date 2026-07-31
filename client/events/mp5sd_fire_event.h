#pragma once
#include "base_game_event.h"
#include "matrix.h"

class CMP5SDFireEvent : public CBaseGameEvent
{
public:
	explicit CMP5SDFireEvent(event_args_t *args);
	void Execute();
private:
	Vector GetShootDirection(const matrix3x3 &camera) const;
};
