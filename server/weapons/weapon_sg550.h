#pragma once
#include "weapons.h"
class CSG550 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CSG550, CBasePlayerWeapon);
public:
	CSG550(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
