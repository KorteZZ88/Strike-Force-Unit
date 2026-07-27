#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

class CColt1911 : public CBasePlayerWeapon
{
	DECLARE_CLASS(CColt1911, CBasePlayerWeapon);
public:
	CColt1911();
	void Spawn();
	void Precache();
};
