#pragma once
#include "base_game_event.h"
class CFamasFireEvent : public CBaseGameEvent { public: explicit CFamasFireEvent(event_args_t* args); void Execute(); };
