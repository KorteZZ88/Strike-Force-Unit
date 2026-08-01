#pragma once
#include "weapons.h"
class CSG552 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CSG552, CBasePlayerWeapon);
public:
	CSG552(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
