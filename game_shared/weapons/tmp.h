#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_TMP 37
#define TMP_WEIGHT 15
#define TMP_MAX_CLIP 30
#define TMP_DEFAULT_GIVE 30
#define TMP_MAX_SPARE_MAGAZINES 5
#define TMP_CLASSNAME weapon_tmp

enum tmp_e
{
	TMP_IDLE = 0,
	TMP_RELOAD,
	TMP_DEPLOY,
	TMP_FIRE1,
	TMP_FIRE2,
	TMP_FIRE3
};

class CTMPWeaponContext : public CBaseWeaponContext
{
public:
	explicit CTMPWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);
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
struct CBaseWeaponContext::AssignedWeaponID<CTMPWeaponContext>
{
	static constexpr int32_t value = WEAPON_TMP;
};
