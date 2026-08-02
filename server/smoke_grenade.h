#pragma once

#include "ggrenade.h"

class CSmokeGrenade : public CGrenade
{
	DECLARE_CLASS(CSmokeGrenade, CGrenade);
public:
	static CSmokeGrenade* ShootTimed(entvars_t* owner, Vector start, Vector velocity, float time);
	void DetonateSmoke();
	void SmokeThink();
	void BounceSound() override;

	float m_flSmokeEnd;
	float m_flModelEnd;
	float m_flNextCloud;
	DECLARE_DATADESC();
};
