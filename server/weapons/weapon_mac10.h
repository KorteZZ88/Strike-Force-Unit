#pragma once
#include "weapons.h"

class CMac10 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CMac10, CBasePlayerWeapon);
public:
	CMac10();
	void Spawn() override;
	void Precache() override;
	int AddToPlayer(CBasePlayer *player) override;
};
