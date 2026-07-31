#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_MP5SD 35
#define MP5SD_WEIGHT 15
#define MP5SD_MAX_CLIP 30
#define MP5SD_MAX_SPARE_MAGAZINES 5
#define MP5SD_DEFAULT_GIVE 30
#define MP5SD_CLASSNAME weapon_mp5sd

enum mp5sd_e
{
	MP5SD_IDLE1,
	MP5SD_RELOAD,
	MP5SD_DEPLOY,
	MP5SD_FIRE1,
	MP5SD_FIRE2,
	MP5SD_FIRE3,
	MP5SD_LONGIDLE = 0,
};

class CMP5SDWeaponContext : public CBaseWeaponContext
{
public:
	CMP5SDWeaponContext() = delete;
	~CMP5SDWeaponContext() = default;
	explicit CMP5SDWeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	void PrimaryAttack() override;
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;

	uint16_t m_usEvent1;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CMP5SDWeaponContext>
{
	static constexpr int32_t value = WEAPON_MP5SD;
};
