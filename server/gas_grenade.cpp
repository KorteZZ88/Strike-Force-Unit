#include "gas_grenade.h"
#include "player.h"
#include "user_messages.h"
LINK_ENTITY_TO_CLASS(gas_grenade, CGasGrenade);
static int g_iPoisonSprite;
BEGIN_DATADESC(CGasGrenade) DEFINE_FIELD(m_flGasEnd,FIELD_TIME), DEFINE_FIELD(m_flNextCloud,FIELD_TIME), DEFINE_FUNCTION(DetonateGas), DEFINE_FUNCTION(GasThink), END_DATADESC()
CGasGrenade* CGasGrenade::ShootTimed(entvars_t* owner,Vector start,Vector velocity,float time) { CGasGrenade* g=GetClassPtr((CGasGrenade*)NULL); g_iPoisonSprite=MODEL_INDEX("sprites/poison.spr"); g->Spawn(); g->pev->classname=MAKE_STRING("gas_grenade"); SET_MODEL(g->edict(),"models/weapon/flashbang/w_flashbang.mdl"); UTIL_SetOrigin(g,start); g->SetAbsVelocity(velocity); g->pev->owner=ENT(owner); g->pev->gravity=.5f; g->pev->friction=.8f; g->SetTouch(&CGrenade::BounceTouch); g->SetThink(&CGasGrenade::DetonateGas); g->SetNextThink(time); return g; }
void CGasGrenade::BounceSound() { EMIT_SOUND(edict(),CHAN_BODY,"weapons/flashbang/flashbang_hit.wav",.7f,ATTN_NORM); }
void CGasGrenade::DetonateGas() { SetAbsVelocity(g_vecZero); pev->solid=SOLID_NOT; pev->effects|=EF_NODRAW; m_flGasEnd=gpGlobals->time+20.f; m_flNextCloud=0; SetThink(&CGasGrenade::GasThink); SetNextThink(0); }
void CGasGrenade::GasThink()
{
	const Vector o=GetAbsOrigin();
	if (gpGlobals->time>=m_flGasEnd) { UTIL_Remove(this); return; }
	if (gpGlobals->time>=m_flNextCloud) { m_flNextCloud=gpGlobals->time+.35f; for(int i=0;i<3;i++) { Vector p=o+Vector(RANDOM_FLOAT(-65,65),RANDOM_FLOAT(-65,65),RANDOM_FLOAT(0,55)); MESSAGE_BEGIN(MSG_PVS,SVC_TEMPENTITY,p); WRITE_BYTE(TE_SPRITE); WRITE_COORD(p.x);WRITE_COORD(p.y);WRITE_COORD(p.z);WRITE_SHORT(g_iPoisonSprite);WRITE_BYTE(RANDOM_LONG(12,22));WRITE_BYTE(RANDOM_LONG(65,115));MESSAGE_END(); } }
	for(int i=1;i<=gpGlobals->maxClients;i++) { CBasePlayer* p=static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i)); if(!p||!p->IsAlive()) continue; if((p->Center()-o).Length()<=200.f) p->TouchGas(pev); }
	CBaseEntity* entity = NULL;
	while((entity = UTIL_FindEntityInSphere(entity, o, 200.0f)) != NULL)
	{
		if(entity->IsPlayer()) continue;
		CBaseMonster* monster = entity->MyMonsterPointer();
		if(monster && monster->IsAlive()) monster->TouchGas(pev);
	}
	SetNextThink(.1f);
}
