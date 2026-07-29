#pragma once
#include "base_game_event.h"

class CDeagleFireEvent : public CBaseGameEvent
{
public:
	explicit CDeagleFireEvent(event_args_t *args);
	void Execute();
};
