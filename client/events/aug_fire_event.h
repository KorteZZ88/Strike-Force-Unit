#pragma once
#include "base_game_event.h"
class CAUGFireEvent : public CBaseGameEvent { public: explicit CAUGFireEvent(event_args_t* args); void Execute(); };
