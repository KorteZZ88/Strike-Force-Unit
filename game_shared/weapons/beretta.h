/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_BERETTA 33
#define BERETTA_WEIGHT		10
#define BERETTA_MAX_CLIP		15
#define BERETTA_MAX_SPARE_MAGAZINES 4
#define BERETTA_DEFAULT_GIVE	15
#define BERETTA_CLASSNAME		weapon_beretta

enum beretta_e
{
	BERETTA_IDLE1 = 0,
	BERETTA_SHOOT,
	BERETTA_SHOOT2,
	BERETTA_SHOOT_EMPTY,
	BERETTA_RELOAD,
	BERETTA_DRAW,
	BERETTA_RELOAD_NOT_EMPTY,
	BERETTA_HOLSTER,
	BERETTA_ADD_SILENCER
};

class CBerettaWeaponContext : public CBaseWeaponContext
{
public:
	CBerettaWeaponContext() = delete;
	CBerettaWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
	~CBerettaWeaponContext() = default;
	
	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	void PrimaryAttack() override;
	bool IsSemiAutomatic() const override { return true; }
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void BerettaFire( float flSpread, float flCycleTime, bool fUseAutoAim );

	uint16_t m_usFireBeretta;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CBerettaWeaponContext> {
	static constexpr int32_t value = WEAPON_BERETTA;
};
