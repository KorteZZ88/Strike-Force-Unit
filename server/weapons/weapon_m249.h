#pragma once
#include "weapons.h"
class CM249 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CM249, CBasePlayerWeapon);
public:
	CM249(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
