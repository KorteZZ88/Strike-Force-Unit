#pragma once
#include "weapons.h"
class CG3SG1 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CG3SG1, CBasePlayerWeapon);
public:
	CG3SG1(); void Spawn() override; void Precache() override; int AddToPlayer(CBasePlayer* player) override;
};
