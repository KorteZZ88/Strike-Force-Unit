#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include "crowbar.h"
#include <memory>

#define WEAPON_WRENCH 18
#define WRENCH_WEIGHT 0
#define WRENCH_CLASSNAME weapon_wrench

class CWrenchWeaponContext : public CBaseWeaponContext
{
public:
	CWrenchWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 3; }
	int GetItemInfo(ItemInfo *p) const override;
	bool Deploy() override;
	void Holster() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;

private:
	void UseTool(bool dismantle);
	void PlaybackEvent();
	uint16_t m_usWrench;
};
