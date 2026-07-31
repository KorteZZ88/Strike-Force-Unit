#pragma once
#include "weapons.h"

class CBizon : public CBasePlayerWeapon
{
	DECLARE_CLASS(CBizon, CBasePlayerWeapon);
public:
	CBizon();
	void Spawn() override;
	void Precache() override;
};
