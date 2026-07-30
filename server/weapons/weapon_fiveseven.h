#pragma once
#include "weapons.h"
class CFiveSeven : public CBasePlayerWeapon
{
public:
	DECLARE_CLASS(CFiveSeven, CBasePlayerWeapon);
	CFiveSeven();
	void Spawn() override;
	void Precache() override;
};
