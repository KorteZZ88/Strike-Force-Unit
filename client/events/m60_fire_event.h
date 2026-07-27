#pragma once
#include "base_game_event.h"
class CM60FireEvent : public CBaseGameEvent { public: explicit CM60FireEvent(event_args_t *args); void Execute(); };
