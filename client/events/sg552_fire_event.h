#pragma once
#include "base_game_event.h"
class CSG552FireEvent : public CBaseGameEvent { public: explicit CSG552FireEvent(event_args_t* args); void Execute(); };
