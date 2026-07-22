#pragma once

#include "weapons.h"

class C556AmmoClip : public CBasePlayerAmmo
{
	DECLARE_CLASS(C556AmmoClip, CBasePlayerAmmo);

	void Spawn() override;
	void Precache() override;
	BOOL AddAmmo(CBaseEntity *other) override;
};
