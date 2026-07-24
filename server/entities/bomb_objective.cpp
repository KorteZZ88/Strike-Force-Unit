#include "bomb_objective.h"
#include "player.h"
#include "gamerules.h"
#include "user_messages.h"
#include "skill.h"
#include "weapons.h"
#include "gamerules/bomb_gamerules.h"
LINK_ENTITY_TO_CLASS(planted_bomb, CObjectiveBomb);
class CBombWinTarget : public CPointEntity
{
	DECLARE_CLASS(CBombWinTarget,CPointEntity);
public:
	void Spawn() override { pev->solid=SOLID_NOT;pev->effects|=EF_NODRAW; }
	void Use(CBaseEntity*,CBaseEntity*,USE_TYPE,float) override { if(g_pGameRules)g_pGameRules->TargetActivated(STRING(pev->targetname)); }
};
LINK_ENTITY_TO_CLASS(bomb_win_target,CBombWinTarget);
BEGIN_DATADESC(CObjectiveBomb)
	DEFINE_FIELD(m_hDefuser, FIELD_EHANDLE), DEFINE_FIELD(m_flDefuseStart, FIELD_TIME), DEFINE_FIELD(m_flExplodeTime, FIELD_TIME),
	DEFINE_FIELD(m_flNextFreq, FIELD_TIME), DEFINE_FIELD(m_flNextFreqInterval, FIELD_FLOAT), DEFINE_FIELD(m_flNextBeep, FIELD_TIME), DEFINE_FIELD(m_iCurWave, FIELD_INTEGER), DEFINE_FUNCTION(BombThink),
END_DATADESC()
void CObjectiveBomb::Precache() { PRECACHE_MODEL("models/weapon/Bomb/w_c4.mdl"); PRECACHE_SOUND("weapons/mine_deploy.wav"); PRECACHE_SOUND("buttons/blip1.wav"); }
void CObjectiveBomb::Spawn()
{
	Precache(); SET_MODEL(edict(), "models/weapon/Bomb/w_c4.mdl"); pev->solid = SOLID_BBOX; pev->movetype = MOVETYPE_TOSS; UTIL_SetSize(pev, Vector(-8,-8,0), Vector(8,8,8));
	const float timer=g_pGameRules?g_pGameRules->BombTimerSeconds():35.0f;
	m_flExplodeTime=gpGlobals->time+timer;m_flNextFreqInterval=Q_max(0.1f,(float)((int)timer/4));m_flNextFreq=gpGlobals->time;m_flNextBeep=gpGlobals->time+0.5f;m_iCurWave=0;m_flDefuseStart=0;SetThink(&CObjectiveBomb::BombThink);pev->nextthink=gpGlobals->time+.1f;
}
void CObjectiveBomb::Use(CBaseEntity *a, CBaseEntity *, USE_TYPE, float)
{
	if (!a || !a->IsPlayer() || !a->IsAlive() || Q_stricmp(a->TeamID(), "blue")) return;
	CBasePlayer *p = static_cast<CBasePlayer*>(a); if ((p->GetAbsOrigin()-GetAbsOrigin()).Length() > 64) return;
	if ((CBaseEntity*)m_hDefuser != p) { m_hDefuser = p; m_flDefuseStart = gpGlobals->time; const bool kit=g_pGameRules&&g_pGameRules->IsBombMode()&&static_cast<CBombGameRules*>(g_pGameRules)->HasDefuseKit(p); EMIT_SOUND(edict(),CHAN_ITEM,"weapons/Bomb/c4_disarm.wav",1.0f,ATTN_NORM); MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,p->pev); WRITE_BYTE(2); WRITE_SHORT(kit?50:100); MESSAGE_END(); }
	Vector velocity = p->GetAbsVelocity();
	velocity.x = velocity.y = 0;
	p->SetAbsVelocity(velocity);
	p->pev->maxspeed = 1;
}
void CObjectiveBomb::BombThink()
{
	static const char *beeps[]={"weapons/Bomb/c4_beep1.wav","weapons/Bomb/c4_beep2.wav","weapons/Bomb/c4_beep3.wav","weapons/Bomb/c4_beep4.wav","weapons/Bomb/c4_beep5.wav"};
	static const float attenuation[]={1.5f,1.0f,0.8f,0.5f,0.2f};
	if(gpGlobals->time>=m_flNextFreq&&m_iCurWave<5){m_flNextFreq=gpGlobals->time+m_flNextFreqInterval;m_flNextFreqInterval*=0.9f;m_iCurWave++;}
	if(gpGlobals->time>=m_flNextBeep){m_flNextBeep=gpGlobals->time+1.4f;const int wave=bound(0,m_iCurWave-1,4);EMIT_SOUND_DYN(edict(),CHAN_VOICE,beeps[wave],1.0f,attenuation[wave],0,PITCH_NORM);}
	CBasePlayer *p = (CBasePlayer*)(CBaseEntity*)m_hDefuser;
	if (p && (!p->IsAlive() || (p->GetAbsOrigin()-GetAbsOrigin()).Length()>64 || !FBitSet(p->pev->button,IN_USE))) { p->pev->maxspeed=0; MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,p->pev); WRITE_BYTE(0); WRITE_SHORT(0); MESSAGE_END(); m_hDefuser=NULL; m_flDefuseStart=0; }
	else if (p) { Vector velocity=p->GetAbsVelocity();velocity.x=velocity.y=0;p->SetAbsVelocity(velocity);p->pev->maxspeed=1; const bool kit=g_pGameRules&&g_pGameRules->IsBombMode()&&static_cast<CBombGameRules*>(g_pGameRules)->HasDefuseKit(p); if (gpGlobals->time-m_flDefuseStart>=(kit?5.0f:10.0f)) { p->pev->maxspeed=0; m_hDefuser=NULL; MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,p->pev); WRITE_BYTE(0); WRITE_SHORT(0); MESSAGE_END(); EMIT_SOUND(edict(),CHAN_ITEM,"weapons/Bomb/c4_disarmed.wav",1.0f,ATTN_NORM); EMIT_SOUND_DYN(edict(),CHAN_VOICE,"radio/bombdef.wav",1.0f,ATTN_NONE,0,PITCH_NORM); if(g_pGameRules)g_pGameRules->BombDefused(p); UTIL_Remove(this); return; } }
	if (gpGlobals->time >= m_flExplodeTime) { Explode(); return; }
	pev->nextthink=gpGlobals->time+.1f;
}
void CObjectiveBomb::Explode()
{
	CBasePlayer *defuser=(CBasePlayer*)(CBaseEntity*)m_hDefuser;
	if(defuser){defuser->pev->maxspeed=0;MESSAGE_BEGIN(MSG_ONE,gmsgActionBar,NULL,defuser->pev);WRITE_BYTE(0);WRITE_SHORT(0);MESSAGE_END();m_hDefuser=NULL;}
	const Vector origin=GetAbsOrigin();
	CBaseEntity *bombTarget=NULL;
	while((bombTarget=UTIL_FindEntityByClassname(bombTarget,"func_bomb_target"))!=NULL)
	{
		if(!Intersects(bombTarget)||FStringNull(bombTarget->pev->target))continue;
		CBaseEntity *planter=pev->owner?CBaseEntity::Instance(pev->owner):this;
		UTIL_FireTargets(STRING(bombTarget->pev->target),planter,bombTarget,USE_TOGGLE,0);
	}
	EMIT_SOUND_DYN(edict(),CHAN_WEAPON,"weapons/Bomb/c4_explode1.wav",1.0f,ATTN_NORM,0,PITCH_NORM);
	pev->solid=SOLID_NOT;pev->takedamage=DAMAGE_NO;pev->effects|=EF_NODRAW;
	MESSAGE_BEGIN(MSG_PAS,SVC_TEMPENTITY,origin);
		WRITE_BYTE(TE_EXPLOSION);WRITE_COORD(origin.x);WRITE_COORD(origin.y);WRITE_COORD(origin.z+8);
		WRITE_SHORT(g_sModelIndexFireball);WRITE_BYTE(40);WRITE_BYTE(15);WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
	// Counter-Strike-style C4 shock: a sharp, violent shake that can be felt
	// considerably farther away than the damaging blast itself.
	UTIL_ScreenShake(origin,25.0f,150.0f,1.0f,3000.0f,true);
	// Classic C4 scale: 500 maximum damage and a 1750-unit blast radius, with
	// linear falloff. Credit the carrier: bomb-mode damage
	// rules allow self-damage while rejecting damage to their teammates.
	entvars_t *attacker=pev->owner?VARS(pev->owner):pev;
	::RadiusDamage(origin,pev,attacker,500.0f,1750.0f,CLASS_NONE,DMG_BLAST);
	if(g_pGameRules)g_pGameRules->BombExploded(this);UTIL_Remove(this);
}
