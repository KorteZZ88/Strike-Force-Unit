#pragma once
#include "weapons.h"

class CM60 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CM60, CBasePlayerWeapon);
public:
	CM60();
	void Spawn() override;
	void Precache() override;
	int AddToPlayer(CBasePlayer *player) override;
};
