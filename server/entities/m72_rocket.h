#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "ggrenade.h"

class CM72Rocket : public CGrenade
{
	DECLARE_CLASS(CM72Rocket, CGrenade);
public:
	void Spawn() override;
	void Precache() override;
	void IgniteThink();
	void FlyThink();
	void RocketTouch(CBaseEntity* other);
	void Explode(TraceResult* trace, int damageType) override;
	static CM72Rocket* Create(const Vector& origin, const Vector& angles, CBaseEntity* owner);

	DECLARE_DATADESC();
private:
	Vector m_vecLaunchOrigin;
	float m_flIgniteTime = 0.0f;
	int m_iTrail = 0;
};
