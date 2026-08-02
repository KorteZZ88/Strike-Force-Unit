#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"

class CSmokeGrenadeWeapon : public CBasePlayerWeapon
{
	DECLARE_CLASS(CSmokeGrenadeWeapon, CBasePlayerWeapon);
public:
	CSmokeGrenadeWeapon();
	void Spawn() override;
	void Precache() override;
};
