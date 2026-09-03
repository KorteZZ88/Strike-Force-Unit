#pragma once

#include "weapons.h"

class CRam : public CBasePlayerWeapon
{
	DECLARE_CLASS( CRam, CBasePlayerWeapon );

public:
	CRam();
	void Spawn();
	void Precache();
};
