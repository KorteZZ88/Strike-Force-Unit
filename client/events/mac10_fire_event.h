#pragma once
#include "base_game_event.h"
#include "matrix.h"

class CMac10FireEvent : public CBaseGameEvent
{
public:
	explicit CMac10FireEvent(event_args_t *args) : CBaseGameEvent(args) {}
	void Execute();
};
