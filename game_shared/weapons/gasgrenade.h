#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_GASGRENADE 21
#define GASGRENADE_WEIGHT 5
#define GASGRENADE_DEFAULT_GIVE 1
#define GASGRENADE_CLASSNAME weapon_gasgrenade

class CGasGrenadeWeaponContext : public CBaseWeaponContext
{
public:
	CGasGrenadeWeaponContext() = delete;
	explicit CGasGrenadeWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 4; }
	int GetItemInfo(ItemInfo* p) const override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	bool CanHolster() override;
	void Holster() override;
	void WeaponIdle() override;
	float m_flStartThrow;
	float m_flReleaseThrow;
	bool m_bWeakThrow;
};
