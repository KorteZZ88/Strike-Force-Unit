#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_COLT1911 27
#define COLT1911_CLASSNAME weapon_1911
#define COLT1911_MAX_CLIP 7
#define COLT1911_DEFAULT_GIVE 7
#define COLT1911_MAX_SPARE_MAGAZINES 4
#define COLT1911_WEIGHT 10

class CColt1911WeaponContext : public CBaseWeaponContext
{
public:
	explicit CColt1911WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	bool IsSemiAutomatic() const override { return true; }
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
private:
	void Fire(float spread);
	float m_flAccuracy = 0.92f;
	float m_flLastFire = 0.0f;
	uint16_t m_usFire = 0;
};
