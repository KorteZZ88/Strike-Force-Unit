#pragma once
#include "base_game_event.h"
class CM249FireEvent : public CBaseGameEvent { public: explicit CM249FireEvent(event_args_t* args); void Execute(); };
