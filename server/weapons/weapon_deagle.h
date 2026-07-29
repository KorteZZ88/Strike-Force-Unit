#pragma once
#include "weapons.h"

class CDeagle : public CBasePlayerWeapon
{
	DECLARE_CLASS(CDeagle, CBasePlayerWeapon);
public:
	CDeagle();
	void Spawn() override;
	void Precache() override;
};
