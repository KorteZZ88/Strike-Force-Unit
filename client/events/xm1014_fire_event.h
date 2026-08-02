#pragma once
#include "base_game_event.h"

class CXM1014FireEvent : public CBaseGameEvent
{
public:
	explicit CXM1014FireEvent(event_args_t* args);
	void Execute();
};
