#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

class CUSP : public CBasePlayerWeapon
{
	DECLARE_CLASS(CUSP, CBasePlayerWeapon);
public:
	CUSP();
	void Spawn();
	void Precache();
};
