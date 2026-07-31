#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_UMP 38
#define UMP_WEIGHT 15
#define UMP_MAX_CLIP 25
#define UMP_DEFAULT_GIVE 25
#define UMP_MAX_SPARE_MAGAZINES 5
#define UMP_CLASSNAME weapon_ump

enum ump_e
{
	UMP_IDLE = 0,
	UMP_RELOAD,
	UMP_DEPLOY,
	UMP_FIRE1,
	UMP_FIRE2,
	UMP_FIRE3
};

class CUMPWeaponContext : public CBaseWeaponContext
{
public:
	explicit CUMPWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
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
struct CBaseWeaponContext::AssignedWeaponID<CUMPWeaponContext>
{
	static constexpr int32_t value = WEAPON_UMP;
};
