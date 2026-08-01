#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_GALIL 41
#define GALIL_WEIGHT 15
#define GALIL_MAX_CLIP 35
#define GALIL_MAX_SPARE_MAGAZINES 4
#define GALIL_DEFAULT_GIVE 35
#define GALIL_CLASSNAME weapon_galil

enum galil_e { GALIL_IDLE, GALIL_RELOAD, GALIL_DRAW, GALIL_SHOOT1, GALIL_SHOOT2, GALIL_SHOOT3 };

class CGalilWeaponContext : public CBaseWeaponContext
{
public:
	CGalilWeaponContext() = delete;
	explicit CGalilWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	bool Deploy() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
private:
	uint16_t m_usFireEvent;
};
