#pragma once
#include "weapons.h"
class CAK47 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CAK47, CBasePlayerWeapon);
public:
	CAK47(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer *player) override;
};
