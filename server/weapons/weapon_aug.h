#pragma once
#include "weapons.h"
class CAUG : public CBasePlayerWeapon
{
	DECLARE_CLASS(CAUG, CBasePlayerWeapon);
public:
	CAUG(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
