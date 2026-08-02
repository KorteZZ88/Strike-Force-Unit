#pragma once

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

class CXM1014 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CXM1014, CBasePlayerWeapon);
public:
	CXM1014();
	void Spawn() override;
	void Precache() override;
	int AddToPlayer(CBasePlayer* player) override;
};
