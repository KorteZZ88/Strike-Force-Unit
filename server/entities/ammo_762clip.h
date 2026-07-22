#pragma once

#include "weapons.h"

class C762AmmoClip : public CBasePlayerAmmo
{
	DECLARE_CLASS(C762AmmoClip, CBasePlayerAmmo);

	void Spawn() override;
	void Precache() override;
	BOOL AddAmmo(CBaseEntity *other) override;
};
