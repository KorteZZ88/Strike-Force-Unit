#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_FLASHBANG 20
#define FLASHBANG_WEIGHT 5
#define FLASHBANG_DEFAULT_GIVE 1
#define FLASHBANG_CLASSNAME weapon_flashbang

class CFlashbangWeaponContext : public CBaseWeaponContext
{
public:
	CFlashbangWeaponContext() = delete;
	explicit CFlashbangWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);

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
	bool m_bQueueNextThrow;
};
