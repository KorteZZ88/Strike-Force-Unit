#include "smoke_grenade.h"

LINK_ENTITY_TO_CLASS(smoke_grenade, CSmokeGrenade);

static int g_iSmokePuffSprite;

class CSmokePuff : public CBaseEntity
{
	DECLARE_CLASS(CSmokePuff, CBaseEntity);
public:
	void Spawn() override
	{
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		pev->rendermode = kRenderTransAlpha;
		pev->renderamt = 0;
		pev->scale = RANDOM_FLOAT(1.5f, 1.85f);
		SET_MODEL(edict(), "sprites/smoke_puff.spr");
		m_flBirthTime = gpGlobals->time;
		m_flDieTime = gpGlobals->time + 20.f;
		m_flMaxAlpha = RANDOM_FLOAT(175.f, 225.f);
		SetThink(&CSmokePuff::PuffThink);
		SetNextThink(.1f);
	}

	void SetMotion(const Vector& center, float radius, float angle)
	{
		m_vecCenter = center;
		m_flInitialRadius = radius;
		m_flInitialAngle = angle;
		m_flRadialSpeed = RANDOM_FLOAT(.6f, 1.5f);
		m_flAngularSpeed = RANDOM_FLOAT(.08f, .18f) * (RANDOM_LONG(0, 1) ? 1.f : -1.f);
		m_flVerticalSpeed = RANDOM_FLOAT(.25f, .8f);
	}

	void PuffThink()
	{
		const float remaining = m_flDieTime - gpGlobals->time;
		if (remaining <= 0.f)
		{
			UTIL_Remove(this);
			return;
		}

		const float age = gpGlobals->time - m_flBirthTime;
		const float radius = m_flInitialRadius + m_flRadialSpeed * age;
		const float angle = m_flInitialAngle + m_flAngularSpeed * age;
		UTIL_SetOrigin(this, m_vecCenter + Vector(
			cos(angle) * radius,
			sin(angle) * radius,
			m_flInitialHeight + m_flVerticalSpeed * age));
		if (age < 1.5f)
			pev->renderamt = m_flMaxAlpha * (age / 1.5f);
		else if (remaining < 2.f)
			pev->renderamt = m_flMaxAlpha * (remaining / 2.f);
		else
			pev->renderamt = m_flMaxAlpha;
		SetNextThink(.1f);
	}

	float m_flBirthTime;
	float m_flDieTime;
	float m_flMaxAlpha;
	Vector m_vecCenter;
	float m_flInitialRadius;
	float m_flInitialAngle;
	float m_flInitialHeight;
	float m_flRadialSpeed;
	float m_flAngularSpeed;
	float m_flVerticalSpeed;
	DECLARE_DATADESC();
};

BEGIN_DATADESC(CSmokePuff)
	DEFINE_FIELD(m_flBirthTime, FIELD_TIME),
	DEFINE_FIELD(m_flDieTime, FIELD_TIME),
	DEFINE_FIELD(m_flMaxAlpha, FIELD_FLOAT),
	DEFINE_FIELD(m_vecCenter, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(m_flInitialRadius, FIELD_FLOAT),
	DEFINE_FIELD(m_flInitialAngle, FIELD_FLOAT),
	DEFINE_FIELD(m_flInitialHeight, FIELD_FLOAT),
	DEFINE_FIELD(m_flRadialSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_flAngularSpeed, FIELD_FLOAT),
	DEFINE_FIELD(m_flVerticalSpeed, FIELD_FLOAT),
	DEFINE_FUNCTION(PuffThink),
END_DATADESC()

BEGIN_DATADESC(CSmokeGrenade)
	DEFINE_FIELD(m_flSmokeEnd, FIELD_TIME),
	DEFINE_FIELD(m_flModelEnd, FIELD_TIME),
	DEFINE_FIELD(m_flNextCloud, FIELD_TIME),
	DEFINE_FUNCTION(DetonateSmoke),
	DEFINE_FUNCTION(SmokeThink),
END_DATADESC()

CSmokeGrenade* CSmokeGrenade::ShootTimed(entvars_t* owner, Vector start, Vector velocity, float time)
{
	CSmokeGrenade* grenade = GetClassPtr((CSmokeGrenade*)NULL);
	g_iSmokePuffSprite = MODEL_INDEX("sprites/smoke_puff.spr");
	grenade->Spawn();
	grenade->pev->classname = MAKE_STRING("smoke_grenade");
	SET_MODEL(grenade->edict(), "models/weapon/Gasgrenade/w_smokegrenade.mdl");
	UTIL_SetOrigin(grenade, start);
	grenade->SetAbsVelocity(velocity);
	grenade->pev->owner = ENT(owner);
	grenade->pev->gravity = .5f;
	grenade->pev->friction = .8f;
	grenade->SetTouch(&CGrenade::BounceTouch);
	grenade->SetThink(&CSmokeGrenade::DetonateSmoke);
	grenade->SetNextThink(time);
	return grenade;
}

void CSmokeGrenade::BounceSound()
{
	EMIT_SOUND(edict(), CHAN_BODY, "weapons/flashbang/flashbang_hit.wav", .7f, ATTN_NORM);
}

void CSmokeGrenade::DetonateSmoke()
{
	SetAbsVelocity(g_vecZero);
	pev->solid = SOLID_NOT;
	m_flSmokeEnd = gpGlobals->time + 20.f;
	m_flModelEnd = gpGlobals->time + 18.f;
	m_flNextCloud = m_flSmokeEnd;
	const Vector origin = GetAbsOrigin();
	constexpr float SMOKE_COVERAGE_RADIUS = 65.f * 1.3f;
	for (int i = 0; i < 42; ++i)
	{
		const float angle = (6.283185307f * i / 42.f) + RANDOM_FLOAT(-.12f, .12f);
		const float radius = RANDOM_FLOAT(12.f, SMOKE_COVERAGE_RADIUS);
		const float height = RANDOM_FLOAT(0, 65);
		Vector puff = origin + Vector(cos(angle) * radius, sin(angle) * radius, height);
		CSmokePuff* cloud = GetClassPtr((CSmokePuff*)NULL);
		UTIL_SetOrigin(cloud, puff);
		cloud->Spawn();
		cloud->m_flInitialHeight = height;
		cloud->SetMotion(origin, radius, angle);
	}
	SetThink(&CSmokeGrenade::SmokeThink);
	SetNextThink(0);
}

void CSmokeGrenade::SmokeThink()
{
	if (gpGlobals->time >= m_flSmokeEnd)
	{
		UTIL_Remove(this);
		return;
	}

	if (gpGlobals->time >= m_flModelEnd)
		pev->effects |= EF_NODRAW;

	SetNextThink(.1f);
}
