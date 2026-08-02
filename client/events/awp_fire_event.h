#pragma once
#include "base_game_event.h"
class CAWPFireEvent : public CBaseGameEvent { public: explicit CAWPFireEvent(event_args_t* args); void Execute(); };
