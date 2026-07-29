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
#include <utility>

#define WEAPON_RAGINGBULL 32
#define RAGINGBULL_WEIGHT			15
#define RAGINGBULL_MAX_CLIP			6
#define RAGINGBULL_MAX_SPARE_MAGAZINES 3
#define RAGINGBULL_DEFAULT_GIVE		6
#define RAGINGBULL_CLASSNAME		weapon_ragingbull

enum ragingbull_e
{
	RAGINGBULL_IDLE1 = 0,
	RAGINGBULL_FIRE1 = 1,
	RAGINGBULL_RELOAD = 2,
	RAGINGBULL_DRAW = 3
};

class CRagingBullWeaponContext : public CBaseWeaponContext
{
public:
	CRagingBullWeaponContext() = delete;
	~CRagingBullWeaponContext() = default;
	CRagingBullWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *p) const override;
	void PrimaryAttack() override;
	bool IsSemiAutomatic() const override { return true; }
	void SecondaryAttack() override;
	bool Deploy() override;
	void Holster() override;
	void Reload() override;
	void WeaponIdle() override;

	bool m_fInZoom;	// don't save this. 
	uint16_t m_usFireRBull;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CRagingBullWeaponContext> {
	static constexpr int32_t value = WEAPON_RAGINGBULL;
};
