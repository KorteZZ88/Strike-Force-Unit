#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_AUG 44
#define AUG_WEIGHT 16
#define AUG_MAX_CLIP 30
#define AUG_MAX_SPARE_MAGAZINES 4
#define AUG_DEFAULT_GIVE 30
#define AUG_CLASSNAME weapon_aug

enum aug_e { AUG_IDLE, AUG_RELOAD, AUG_DRAW, AUG_SHOOT1, AUG_SHOOT2, AUG_SHOOT3 };

class CAUGWeaponContext : public CBaseWeaponContext
{
public:
	CAUGWeaponContext() = delete;
	explicit CAUGWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	bool Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
private:
	uint16_t m_usFireEvent;
};
