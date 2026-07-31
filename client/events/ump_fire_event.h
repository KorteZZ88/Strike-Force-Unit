#pragma once
#include "base_game_event.h"

class CUMPFireEvent : public CBaseGameEvent
{
public:
	explicit CUMPFireEvent(event_args_t *args) : CBaseGameEvent(args) {}
	void Execute();
};
