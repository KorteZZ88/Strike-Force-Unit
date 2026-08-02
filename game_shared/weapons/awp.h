#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_AWP 48
#define AWP_CLASSNAME weapon_awp
#define AWP_MAX_CLIP 5
#define AWP_DEFAULT_GIVE 5
#define AWP_MAX_SPARE_MAGAZINES 2
#define AWP_WEIGHT 25

enum awp_e { AWP_IDLE, AWP_FIRE1, AWP_FIRE2, AWP_FIRE3, AWP_RELOAD, AWP_DRAW };

class CAWPWeaponContext : public CBaseWeaponContext
{
public:
	explicit CAWPWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 4; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
	bool IsSemiAutomatic() const override { return true; }
	bool ShouldWeaponIdle() override { return true; }

private:
	uint16_t m_usFireEvent = 0;
	float m_flRestoreZoomFov = 0.0f;
};
