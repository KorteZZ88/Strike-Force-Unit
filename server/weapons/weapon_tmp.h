#pragma once
#include "weapons.h"

class CTMP : public CBasePlayerWeapon
{
	DECLARE_CLASS(CTMP, CBasePlayerWeapon);
public:
	CTMP();
	void Spawn() override;
	void Precache() override;
};
