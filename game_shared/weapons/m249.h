#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_M249 49
#define M249_WEIGHT 25
#define M249_MAX_CLIP 100
#define M249_MAX_SPARE_MAGAZINES 2
#define M249_DEFAULT_GIVE 100
#define M249_CLASSNAME weapon_m249

enum m249_e { M249_IDLE, M249_SHOOT1, M249_SHOOT2, M249_RELOAD, M249_DRAW };

class CM249WeaponContext : public CBaseWeaponContext
{
public:
	explicit CM249WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	bool Deploy() override;
	void PrimaryAttack() override;
	void PrimaryAttackReleased() override { m_iShotsFired = 0; }
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
private:
	uint16_t m_usFireEvent;
	int m_iShotsFired = 0;
};
