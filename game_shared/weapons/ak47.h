#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_AK47 24
#define AK47_WEIGHT 15
#define AK47_MAX_CLIP 30
#define AK47_MAX_SPARE_MAGAZINES 6
#define AK47_DEFAULT_GIVE 30
#define AK47_CLASSNAME weapon_ak47

enum ak47_e { AK47_IDLE, AK47_RELOAD, AK47_DRAW, AK47_SHOOT1, AK47_SHOOT2, AK47_SHOOT3 };

class CAK47WeaponContext : public CBaseWeaponContext
{
public:
	CAK47WeaponContext() = delete;
	explicit CAK47WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *info) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	bool Deploy() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
private:
	uint16_t m_usFireEvent;
};
