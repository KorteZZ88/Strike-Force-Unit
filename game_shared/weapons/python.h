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

#define WEAPON_RBULL			3
#define RBULL_WEIGHT			15
#define RBULL_MAX_CLIP			6
#define RBULL_MAX_SPARE_MAGAZINES 3
#define RBULL_DEFAULT_GIVE		6
#define RBULL_CLASSNAME		weapon_rbull

enum rbull_e
{
	RBULL_IDLE1 = 0,
	RBULL_FIRE1 = 1,
	RBULL_RELOAD = 2,
	RBULL_DRAW = 3
};

class CRBullWeaponContext : public CBaseWeaponContext
{
public:
	CRBullWeaponContext() = delete;
	~CRBullWeaponContext() = default;
	CRBullWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

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
