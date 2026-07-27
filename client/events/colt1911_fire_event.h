#pragma once
#include "base_game_event.h"

class CColt1911FireEvent : public CBaseGameEvent
{
public:
	explicit CColt1911FireEvent(event_args_t *args);
	void Execute();
};
