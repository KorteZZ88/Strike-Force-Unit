#pragma once
#include "weapons.h"
class CAWP : public CBasePlayerWeapon
{
public:
	DECLARE_CLASS(CAWP, CBasePlayerWeapon);
	CAWP(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
