#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_M60 26
#define M60_WEIGHT 20
#define M60_MAX_CLIP 100
#define M60_MAX_SPARE_MAGAZINES 2
#define M60_DEFAULT_GIVE 100
#define M60_CLASSNAME weapon_m60

enum m60_e { M60_IDLE, M60_SHOOT1, M60_SHOOT2, M60_RELOAD, M60_DRAW };

class CM60WeaponContext : public CBaseWeaponContext
{
public:
	CM60WeaponContext() = delete;
	explicit CM60WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *info) const override;
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
