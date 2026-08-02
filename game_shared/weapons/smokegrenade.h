#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_SMOKEGRENADE 45
#define SMOKEGRENADE_WEIGHT 5
#define SMOKEGRENADE_DEFAULT_GIVE 1
#define SMOKEGRENADE_CLASSNAME weapon_smokegrenade

class CSmokeGrenadeWeaponContext : public CBaseWeaponContext
{
public:
	CSmokeGrenadeWeaponContext() = delete;
	explicit CSmokeGrenadeWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
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
