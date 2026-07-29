/*
*
*	Copyright(c) 1996 - 2002, Valve LLC.All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").Id Technology(c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and /or resulting
* object code is restricted to non - commercial enhancements to products from
* Valve LLC.All other use, distribution, or modification is prohibited
* without written permission from Valve LLC.
*
****/

#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>
#include <utility>

#define WEAPON_M3 31
#define M3_WEIGHT			15
#define M3_MAGAZINE_SIZE	7
#define M3_MAX_CLIP		8 // seven in the tube plus one chambered
#define M3_DEFAULT_GIVE	7
#define M3_CLASSNAME		weapon_m3

enum m3_e
{
	M3_IDLE = 0,
	M3_FIRE,
	M3_FIRE2,
	M3_RELOAD,
	M3_PUMP,
	M3_START_RELOAD,
	M3_DRAW,
	M3_HOLSTER,
	M3_IDLE4,
	M3_IDLE_DEEP
};

class CM3WeaponContext : public CBaseWeaponContext
{
public:
	CM3WeaponContext() = delete;
	~CM3WeaponContext() = default;
	CM3WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *p) const override;
	void PrimaryAttack() override;
	//void SecondaryAttack() override;
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;

	uint16_t m_usSingleFire;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CM3WeaponContext> {
	static constexpr int32_t value = WEAPON_M3;
};
