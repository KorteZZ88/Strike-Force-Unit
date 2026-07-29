#pragma once
#include "base_game_event.h"
class CP229FireEvent : public CBaseGameEvent { public: explicit CP229FireEvent(event_args_t *args); void Execute(); };
