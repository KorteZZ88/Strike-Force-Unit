#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_MAC10 36
#define MAC10_WEIGHT 15
#define MAC10_MAX_CLIP 30
#define MAC10_DEFAULT_GIVE 30
#define MAC10_MAX_SPARE_MAGAZINES 6
#define MAC10_CLASSNAME weapon_mac10

enum mac10_e
{
	MAC10_IDLE1 = 0,
	MAC10_RELOAD,
	MAC10_DEPLOY,
	MAC10_FIRE1,
	MAC10_FIRE2,
	MAC10_FIRE3
};

class CMac10WeaponContext : public CBaseWeaponContext
{
public:
	explicit CMac10WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
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
struct CBaseWeaponContext::AssignedWeaponID<CMac10WeaponContext>
{
	static constexpr int32_t value = WEAPON_MAC10;
};
