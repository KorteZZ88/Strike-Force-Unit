#pragma once
#include "base_game_event.h"
class CUSPFireEvent : public CBaseGameEvent { public: explicit CUSPFireEvent(event_args_t *args); void Execute(); };
