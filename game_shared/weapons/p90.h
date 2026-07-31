#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_P90 39
#define P90_WEIGHT 15
#define P90_MAX_CLIP 50
#define P90_DEFAULT_GIVE 50
#define P90_MAX_SPARE_MAGAZINES 2
#define P90_CLASSNAME weapon_p90

enum p90_e
{
	P90_IDLE = 0,
	P90_RELOAD_EMPTY,
	P90_DEPLOY,
	P90_FIRE1,
	P90_FIRE2,
	P90_FIRE3
};

class CP90WeaponContext : public CBaseWeaponContext
{
public:
	explicit CP90WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

private:
	uint16_t m_usEvent = 0;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CP90WeaponContext>
{
	static constexpr int32_t value = WEAPON_P90;
};
