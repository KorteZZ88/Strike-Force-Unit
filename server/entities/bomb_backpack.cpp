#include "cbase.h"
#include "player.h"
class CBombBackpack:public CBaseEntity
{
public:void Spawn()override{PRECACHE_MODEL("models/weapon/Bomb/w_c4.mdl");SET_MODEL(edict(),"models/weapon/Bomb/w_c4.mdl");pev->solid=SOLID_NOT;pev->movetype=MOVETYPE_NOCLIP;SetThink(&CBombBackpack::FollowThink);pev->nextthink=gpGlobals->time;}
	void FollowThink(){CBasePlayer*p=(CBasePlayer*)CBaseEntity::Instance(pev->owner);if(!p||!p->IsAlive()||!p->HasNamedPlayerItem("weapon_bomb")){UTIL_Remove(this);return;}UTIL_MakeVectors(Vector(0,p->pev->angles.y,0));SetAbsOrigin(p->GetAbsOrigin()-gpGlobals->v_forward*9+Vector(0,0,38));SetAbsAngles(Vector(0,p->pev->angles.y+90,90));pev->nextthink=gpGlobals->time+.03f;}
};
LINK_ENTITY_TO_CLASS(bomb_backpack,CBombBackpack);
