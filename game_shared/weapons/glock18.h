#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

// ID 5 is the first unused legacy weapon slot. Keep custom weapons inside the
// original engine-side weapon table used by this Xash3D build.
#define WEAPON_GLOCK18 5
#define GLOCK18_CLASSNAME weapon_glock18
#define GLOCK18_MAX_CLIP 20
#define GLOCK18_DEFAULT_GIVE 20
#define GLOCK18_MAX_SPARE_MAGAZINES 4
#define GLOCK18_WEIGHT 10

// Sequence indices are defined by models/weapon/glock18/v_glock18.mdl.
enum glock18_e { GLOCK18_IDLE = 0, GLOCK18_SHOOT = 3, GLOCK18_SHOOT_EMPTY = 6, GLOCK18_RELOAD = 7, GLOCK18_DRAW = 8 };

class CGlock18WeaponContext : public CBaseWeaponContext
{
public:
	explicit CGlock18WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	bool IsSemiAutomatic() const override { return !m_bFullAuto; }
	bool IsFullAuto() const { return m_bFullAuto; }
	void SetFullAuto(bool value) { m_bFullAuto = value; }
private:
	void Fire(float spread, float cycleTime);
	bool m_bFullAuto = false;
	uint16_t m_usFire = 0;
};
