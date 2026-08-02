#pragma once
#include "base_game_event.h"
class CG3SG1FireEvent : public CBaseGameEvent { public: explicit CG3SG1FireEvent(event_args_t* args); void Execute(); };
