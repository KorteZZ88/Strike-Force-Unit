#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_M4			17
#define M4_WEIGHT			15
#define M4_MAX_CLIP			30
#define M4_DEFAULT_AMMO		30
#define M4_DEFAULT_GIVE		30
#define M4_CLASSNAME		weapon_m4

enum m4_e
{
	M4_IDLE11,
	M4_FIRE11,
	M4_FIRE21,
	M4_FIRE31,
	M4_RELOAD1,
	M4_DEPLOY1,
	M4_LAUNCH1,
	M4_IDLE,
	M4_FIRE1,
	M4_FIRE2,
	M4_FIRE3,
	M4_RELOAD,
	M4_DEPLOY,
	M4_LAUNCH,
};

class CM4WeaponContext : public CBaseWeaponContext
{
public:
	CM4WeaponContext() = delete;
	~CM4WeaponContext() = default;
	CM4WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	int SecondaryAmmoIndex() override;
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;

	uint16_t m_usEvent1;
	uint16_t m_usEvent2;

};
