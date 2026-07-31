#pragma once
#include "base_game_event.h"

class CTMPFireEvent : public CBaseGameEvent
{
public:
	explicit CTMPFireEvent(event_args_t *args) : CBaseGameEvent(args) {}
	void Execute();
};
