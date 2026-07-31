#pragma once
#include "weapons.h"

class CMP5SD : public CBasePlayerWeapon
{
	DECLARE_CLASS(CMP5SD, CBasePlayerWeapon);
public:
	CMP5SD();
	void Spawn() override;
	void Precache() override;
	int AddToPlayer(CBasePlayer *pPlayer) override;
};
