#pragma once
#include "weapons.h"

class CP229 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CP229, CBasePlayerWeapon);
public:
	CP229();
	void Spawn() override;
	void Precache() override;
};
