#pragma once
#include "weapons.h"
class CFamas : public CBasePlayerWeapon
{
	DECLARE_CLASS(CFamas, CBasePlayerWeapon);
public:
	CFamas(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
