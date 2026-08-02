#pragma once
#include "base_game_event.h"
class CSG550FireEvent : public CBaseGameEvent { public: explicit CSG550FireEvent(event_args_t* args); void Execute(); };
