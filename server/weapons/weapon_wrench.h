#pragma once

#include "weapons.h"

class CWrench : public CBasePlayerWeapon
{
	DECLARE_CLASS(CWrench, CBasePlayerWeapon);
public:
	CWrench();
	void Spawn();
	void Precache();
};
