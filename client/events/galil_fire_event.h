#pragma once
#include "base_game_event.h"
class CGalilFireEvent : public CBaseGameEvent { public: explicit CGalilFireEvent(event_args_t* args); void Execute(); };
