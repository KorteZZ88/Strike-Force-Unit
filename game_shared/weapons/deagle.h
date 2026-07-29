#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_DEAGLE 29
#define DEAGLE_CLASSNAME weapon_deagle
#define DEAGLE_MAX_CLIP 7
#define DEAGLE_DEFAULT_GIVE 7
#define DEAGLE_MAX_SPARE_MAGAZINES 4
#define DEAGLE_WEIGHT 15

enum deagle_e
{
	DEAGLE_IDLE = 0,
	DEAGLE_SHOOT1,
	DEAGLE_SHOOT2,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_DRAW
};

class CDeagleWeaponContext : public CBaseWeaponContext
{
public:
	explicit CDeagleWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *info) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	bool IsSemiAutomatic() const override { return true; }
	void Reload() override;
	void WeaponIdle() override;

private:
	uint16_t m_usFireDeagle = 0;
};
