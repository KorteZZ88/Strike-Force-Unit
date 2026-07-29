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
#include <utility>

#define WEAPON_MP5A3 30
#define MP5A3_WEIGHT			15
#define MP5A3_MAX_CLIP		30
#define MP5A3_DEFAULT_AMMO	30
#define MP5A3_DEFAULT_GIVE	30
#define MP5A3_CLASSNAME		weapon_mp5a3

enum mp5a3_e
{
	MP5A3_IDLE1,
	MP5A3_RELOAD,
	MP5A3_DEPLOY,
	MP5A3_FIRE1,
	MP5A3_FIRE2,
	MP5A3_FIRE3,
	MP5A3_LONGIDLE = 0,
};

class CMP5A3WeaponContext : public CBaseWeaponContext
{
public:
	CMP5A3WeaponContext() = delete;
	~CMP5A3WeaponContext() = default;
	CMP5A3WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	void PrimaryAttack() override;
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;

	uint16_t m_usEvent1;
	uint16_t m_usEvent2;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CMP5A3WeaponContext> {
	static constexpr int32_t value = WEAPON_MP5A3;
};
