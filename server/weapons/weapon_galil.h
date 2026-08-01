#pragma once
#include "weapons.h"
class CGalil : public CBasePlayerWeapon
{
	DECLARE_CLASS(CGalil, CBasePlayerWeapon);
public:
	CGalil(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
