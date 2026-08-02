#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_XM1014 50
#define XM1014_WEIGHT 15
#define XM1014_MAGAZINE_SIZE 6
#define XM1014_MAX_CLIP 7
#define XM1014_DEFAULT_GIVE 6
#define XM1014_MAX_CARRY 24
#define XM1014_CLASSNAME weapon_xm1014

enum xm1014_e
{
	XM1014_IDLE = 0,
	XM1014_FIRE1,
	XM1014_FIRE2,
	XM1014_RELOAD,
	XM1014_AFTER_RELOAD,
	XM1014_START_RELOAD,
	XM1014_DRAW
};

class CXM1014WeaponContext : public CBaseWeaponContext
{
public:
	explicit CXM1014WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);

	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo* info) const override;
	bool Deploy() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

private:
	uint16_t m_usFireEvent = 0;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CXM1014WeaponContext>
{
	static constexpr int32_t value = WEAPON_XM1014;
};
