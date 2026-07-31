#pragma once
#include "weapons.h"

class CUMP : public CBasePlayerWeapon
{
	DECLARE_CLASS(CUMP, CBasePlayerWeapon);
public:
	CUMP();
	void Spawn() override;
	void Precache() override;
};
