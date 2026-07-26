#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

class CM72 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CM72, CBasePlayerWeapon);
public:
	CM72();
	void Spawn() override;
	void Precache() override;
	int AddToPlayer(CBasePlayer* player) override;
	void RetireSpentLauncher();
};
