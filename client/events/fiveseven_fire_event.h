#pragma once
#include "base_game_event.h"
class CFiveSevenFireEvent : public CBaseGameEvent { public: explicit CFiveSevenFireEvent(event_args_t *args); void Execute(); };
