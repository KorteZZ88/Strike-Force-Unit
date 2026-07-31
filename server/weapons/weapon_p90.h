#pragma once
#include "weapons.h"

class CP90 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CP90, CBasePlayerWeapon);
public:
	CP90();
	void Spawn() override;
	void Precache() override;
};
