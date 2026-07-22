#pragma once
#include "weapons.h"
class C762x39AmmoClip : public CBasePlayerAmmo
{
	DECLARE_CLASS(C762x39AmmoClip, CBasePlayerAmmo);
public: void Spawn() override; void Precache() override; BOOL AddAmmo(CBaseEntity *other) override;
};
