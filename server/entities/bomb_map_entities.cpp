#include "cbase.h"
#include "triggers.h"
class CBombMapPoint:public CPointEntity{}; class CBombTarget:public CBaseTrigger{public:void Spawn()override{InitTrigger();pev->solid=SOLID_TRIGGER;}};
LINK_ENTITY_TO_CLASS(info_player_red,CBombMapPoint); LINK_ENTITY_TO_CLASS(info_player_blue,CBombMapPoint); LINK_ENTITY_TO_CLASS(func_bomb_target,CBombTarget);
