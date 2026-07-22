#pragma once
#include "weapons.h"
class CBombWeapon : public CBasePlayerWeapon { DECLARE_CLASS(CBombWeapon, CBasePlayerWeapon); public: CBombWeapon(); void Spawn() override; void Precache() override; };
