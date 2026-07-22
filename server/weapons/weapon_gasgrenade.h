#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
class CGasGrenadeWeapon : public CBasePlayerWeapon
{
	DECLARE_CLASS(CGasGrenadeWeapon, CBasePlayerWeapon);
public:
	CGasGrenadeWeapon(); void Spawn() override; void Precache() override;
};
