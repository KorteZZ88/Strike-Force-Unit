#include "m72_rocket.h"

LINK_ENTITY_TO_CLASS(m72_rocket, CM72Rocket);

BEGIN_DATADESC(CM72Rocket)
	DEFINE_FIELD(m_vecLaunchOrigin, FIELD_VECTOR),
	DEFINE_FIELD(m_flIgniteTime, FIELD_TIME),
	DEFINE_FUNCTION(IgniteThink),
	DEFINE_FUNCTION(FlyThink),
	DEFINE_FUNCTION(RocketTouch),
END_DATADESC()

CM72Rocket* CM72Rocket::Create(const Vector& origin, const Vector& angles, CBaseEntity* owner)
{
	CM72Rocket* rocket = GetClassPtr((CM72Rocket*)NULL);
	UTIL_SetOrigin(rocket, origin);
	rocket->SetLocalAngles(angles);
	rocket->Spawn();
	rocket->m_vecLaunchOrigin = origin;
	rocket->pev->owner = owner->edict();
	return rocket;
}

void CM72Rocket::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	SET_MODEL(edict(), "models/weapon/m72/lawrocket.mdl");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	SetTouch(&CM72Rocket::RocketTouch);
	SetThink(&CM72Rocket::IgniteThink);
	SetNextThink(0.4f);

	Vector angles = GetLocalAngles();
	angles.x -= 30.0f;
	UTIL_MakeVectors(angles);
	SetLocalVelocity(gpGlobals->v_forward * 250.0f);
	pev->gravity = 0.5f;
	pev->dmg = gSkillData.plrDmgM72;
}

void CM72Rocket::Precache()
{
	PRECACHE_MODEL("models/weapon/m72/lawrocket.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("weapons/rocket1.wav");
}

void CM72Rocket::IgniteThink()
{
	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;
	EMIT_SOUND(edict(), CHAN_VOICE, "weapons/rocket1.wav", 1.0f, 0.5f);

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW); WRITE_SHORT(entindex()); WRITE_SHORT(m_iTrail);
	WRITE_BYTE(40); WRITE_BYTE(5); WRITE_BYTE(224); WRITE_BYTE(224); WRITE_BYTE(255); WRITE_BYTE(255);
	MESSAGE_END();

	m_flIgniteTime = gpGlobals->time;
	SetThink(&CM72Rocket::FlyThink);
	SetNextThink(0.1f);
}

void CM72Rocket::FlyThink()
{
	// This is Half-Life CRpgRocket::FollowThink without the laser-spot scan.
	Vector angles = GetLocalAngles();
	UTIL_MakeVectors(angles);
	const Vector vecTarget = gpGlobals->v_forward;
	SetLocalAngles(UTIL_VecToAngles(vecTarget));

	const float speed = GetLocalVelocity().Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0f)
	{
		SetLocalVelocity(GetLocalVelocity() * 0.2f + vecTarget * (speed * 0.8f + 400.0f));
		if (pev->waterlevel == 3)
		{
			if (GetLocalVelocity().Length() > 300.0f)
				SetLocalVelocity(GetLocalVelocity().Normalize() * 300.0f);
			UTIL_BubbleTrail(GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 4);
		}
		else if (GetLocalVelocity().Length() > 2000.0f)
		{
			SetLocalVelocity(GetLocalVelocity().Normalize() * 2000.0f);
		}
	}
	else
	{
		if (pev->effects & EF_LIGHT)
		{
			pev->effects = 0;
			STOP_SOUND(edict(), CHAN_VOICE, "weapons/rocket1.wav");
		}
		SetLocalVelocity(GetLocalVelocity() * 0.2f + vecTarget * speed * 0.798f);
		if (pev->waterlevel == 0 && GetLocalVelocity().Length() < 1500.0f)
		{
			Detonate();
			return;
		}
	}

	if (UTIL_PointContents(GetAbsOrigin()) == CONTENTS_SKY || (GetAbsOrigin() - m_vecLaunchOrigin).Length() > 92680.0f)
		UTIL_Remove(this);
	else
		SetNextThink(0.1f);
}

void CM72Rocket::RocketTouch(CBaseEntity*)
{
	// Five metres in GoldSrc units (1 unit = 1 inch). A close impact is a dud.
	if ((GetAbsOrigin() - m_vecLaunchOrigin).Length() < 196.85f)
	{
		STOP_SOUND(edict(), CHAN_VOICE, "weapons/rocket1.wav");
		MESSAGE_BEGIN(MSG_ALL, SVC_TEMPENTITY);
			WRITE_BYTE(TE_KILLBEAM);
			WRITE_ENTITY(entindex());
		MESSAGE_END();
		pev->effects &= ~EF_LIGHT;
		pev->movetype = MOVETYPE_TOSS;
		pev->gravity = 1.0f;
		SetLocalVelocity(Vector(0, 0, -100.0f));
		SetLocalAvelocity(g_vecZero);
		SetTouch(NULL);
		SetThink(NULL);
		return;
	}
	TraceResult trace;
	UTIL_TraceLine(GetAbsOrigin() - GetAbsVelocity().Normalize() * 16.0f, GetAbsOrigin() + GetAbsVelocity().Normalize() * 32.0f, dont_ignore_monsters, edict(), &trace);
	Explode(&trace, DMG_BLAST);
}

void CM72Rocket::Explode(TraceResult* trace, int damageType)
{
	STOP_SOUND(edict(), CHAN_VOICE, "weapons/rocket1.wav");
	CGrenade::Explode(trace, damageType);
}
