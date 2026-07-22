#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

class CM4 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CM4, CBasePlayerWeapon);

public:
	CM4();

	void Spawn();
	void Precache();
	int AddToPlayer(CBasePlayer *pPlayer);
};