#pragma once
#include "base_game_event.h"
class CAK47FireEvent : public CBaseGameEvent { public: explicit CAK47FireEvent(event_args_t *args); void Execute(); };
