#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_BIZON 40
#define BIZON_WEIGHT 15
#define BIZON_MAX_CLIP 64
#define BIZON_DEFAULT_GIVE 64
#define BIZON_MAX_SPARE_MAGAZINES 2
#define BIZON_CLASSNAME weapon_bizon

enum bizon_e
{
	BIZON_IDLE = 0,
	BIZON_RELOAD,
	BIZON_DEPLOY,
	BIZON_FIRE1,
	BIZON_FIRE2,
	BIZON_FIRE3
};

class CBizonWeaponContext : public CBaseWeaponContext
{
public:
	explicit CBizonWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
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
struct CBaseWeaponContext::AssignedWeaponID<CBizonWeaponContext>
{
	static constexpr int32_t value = WEAPON_BIZON;
};
