#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_FIVESEVEN 34
#define FIVESEVEN_CLASSNAME weapon_fiveseven
#define FIVESEVEN_MAX_CLIP 20
#define FIVESEVEN_DEFAULT_GIVE 20
#define FIVESEVEN_MAX_SPARE_MAGAZINES 3
#define FIVESEVEN_WEIGHT 10

enum fiveseven_e
{
	FIVESEVEN_IDLE = 0,
	FIVESEVEN_SHOOT1,
	FIVESEVEN_SHOOT2,
	FIVESEVEN_SHOOT_LAST,
	FIVESEVEN_RELOAD,
	FIVESEVEN_DRAW
};

class CFiveSevenWeaponContext : public CBaseWeaponContext
{
public:
	explicit CFiveSevenWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	bool IsSemiAutomatic() const override { return true; }
	void Reload() override;
	void WeaponIdle() override;

private:
	uint16_t m_usFireFiveSeven = 0;
};
