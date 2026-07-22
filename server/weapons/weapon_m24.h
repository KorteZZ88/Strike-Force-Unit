
#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

class CM24 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CM24, CBasePlayerWeapon);

public:
	CM24();

	void Spawn();
	void Precache();
	int AddToPlayer(CBasePlayer *pPlayer);
};
