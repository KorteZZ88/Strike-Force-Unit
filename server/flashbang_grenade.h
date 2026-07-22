#pragma once
#include "ggrenade.h"

class CFlashbangGrenade : public CGrenade
{
	DECLARE_CLASS(CFlashbangGrenade, CGrenade);
public:
	static CFlashbangGrenade* ShootTimed(entvars_t* owner, Vector start, Vector velocity, float time);
	void DetonateFlash();
	void BounceSound() override;
	DECLARE_DATADESC();
};
