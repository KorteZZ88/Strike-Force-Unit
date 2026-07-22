#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_C4                   19
#define C4_WEIGHT                   -10
#define C4_DEFAULT_GIVE             1
#define C4_CLASSNAME                weapon_c4

enum timed_satchel_e
{
	C4_IDLE = 0,
	C4_FIDGET,
	C4_DRAW,
	C4_PLACE
};

class CTimedSatchelWeaponContext : public CBaseWeaponContext
{
public:
	CTimedSatchelWeaponContext() = delete;
	explicit CTimedSatchelWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);

	int iItemSlot() override { return 4; }
	int GetItemInfo(ItemInfo *p) const override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	bool Deploy() override;
	void Holster() override;
	void WeaponIdle() override;

	int m_iTimerSeconds;
};
