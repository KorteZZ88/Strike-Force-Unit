#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_P229 28
#define P229_CLASSNAME weapon_p229
#define P229_MAX_CLIP 12
#define P229_DEFAULT_GIVE 12
#define P229_MAX_SPARE_MAGAZINES 4
#define P229_WEIGHT 10

enum p229_e
{
	P229_IDLE = 0,
	P229_SHOOT1,
	P229_SHOOT2,
	P229_SHOOT3,
	P229_SHOOT_EMPTY,
	P229_RELOAD,
	P229_DRAW
};

class CP229WeaponContext : public CBaseWeaponContext
{
public:
	explicit CP229WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	bool IsSemiAutomatic() const override { return true; }
	void Reload() override;
	void WeaponIdle() override;

private:
	uint16_t m_usFireP229 = 0;
};
