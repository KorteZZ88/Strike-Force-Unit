#pragma once
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"

class CFlashbang : public CBasePlayerWeapon
{
	DECLARE_CLASS(CFlashbang, CBasePlayerWeapon);
public:
	CFlashbang();
	void Spawn() override;
	void Precache() override;
	void RememberWeaponBeforeFlashbang(CBasePlayerItem* weapon);
	BOOL RestoreWeaponBeforeFlashbang();

	CBasePlayerItem* m_pWeaponBeforeFlashbang;
	DECLARE_DATADESC();
};
