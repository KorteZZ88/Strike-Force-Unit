#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_M72 25
#define M72_WEIGHT 6
#define M72_CLASSNAME weapon_m72
#define M72_DRAW_TIME (92.0f / 30.0f)
#define M72_SHOOT_TIME (33.0f / 30.0f)

enum m72_e
{
	M72_IDLE = 0,
	M72_DRAW,
	M72_SHOOT,
	M72_DISCARD,
};

class CM72WeaponContext : public CBaseWeaponContext
{
public:
	CM72WeaponContext() = delete;
	explicit CM72WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);

	int iItemSlot() override { return 4; }
	int GetItemInfo(ItemInfo* p) const override;
	bool Deploy() override;
	void Holster() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override {}
	void Reload() override {}
	void WeaponIdle() override;

private:
	bool m_bSpent = false;
};
