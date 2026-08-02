#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_G3SG1 46
#define G3SG1_CLASSNAME weapon_g3sg1
#define G3SG1_MAX_CLIP 20
#define G3SG1_DEFAULT_GIVE 20
#define G3SG1_MAX_SPARE_MAGAZINES 3
#define G3SG1_WEIGHT 20

enum g3sg1_e { G3SG1_IDLE, G3SG1_SHOOT1, G3SG1_SHOOT2, G3SG1_RELOAD, G3SG1_DRAW };

class CG3SG1WeaponContext : public CBaseWeaponContext
{
public:
	explicit CG3SG1WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
	bool IsSemiAutomatic() const override { return true; }

private:
	uint16_t m_usFireEvent = 0;
};
