#pragma once
#include "weapons.h"
class CC2 : public CBasePlayerWeapon
{
	DECLARE_CLASS( CC2, CBasePlayerWeapon );
public:
	CC2();
	void Spawn();
	void Precache();
	DECLARE_DATADESC();
};
