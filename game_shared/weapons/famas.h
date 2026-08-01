#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_FAMAS 42
#define FAMAS_WEIGHT 15
#define FAMAS_MAX_CLIP 25
#define FAMAS_MAX_SPARE_MAGAZINES 4
#define FAMAS_DEFAULT_GIVE 25
#define FAMAS_CLASSNAME weapon_famas

enum famas_e { FAMAS_IDLE, FAMAS_RELOAD, FAMAS_DRAW, FAMAS_SHOOT1, FAMAS_SHOOT2, FAMAS_SHOOT3 };

class CFamasWeaponContext : public CBaseWeaponContext
{
public:
	CFamasWeaponContext() = delete;
	explicit CFamasWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	bool Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void ProcessBurstShots();
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
	bool IsSemiAutomatic() const override { return m_bBurstMode; }
	bool IsBurstMode() const { return m_bBurstMode; }
	void SetBurstMode(bool burst) { m_bBurstMode = burst; }
private:
	void FireShot(int burstBullet);
	bool m_bBurstMode = false;
	int m_iBurstShotsRemaining = 0;
	uint16_t m_usFireEvent;
};
