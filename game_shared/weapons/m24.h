#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_M24			16
#define M24_WEIGHT			15
#define M24_MAX_CLIP		10
#define M24_MAX_SPARE_MAGAZINES 2
#define M24_DEFAULT_AMMO	10
#define M24_DEFAULT_GIVE	10
#define M24_CLASSNAME		weapon_m24

enum m24_e
{
	M24_IDLE,
	M24_SHOOT1,
	M24_SHOOT2,
	M24_RELOAD,
	M24_DEPLOY,
	M24_HOLSTER,
};

class CM24WeaponContext : public CBaseWeaponContext
{
public:
	CM24WeaponContext() = delete;
	~CM24WeaponContext() = default;
	CM24WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Holster() override;
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;

	bool m_fInZoom;	// don't save this. 
	bool m_fInReload = false;
	
	int m_reloadAmount = 0;

	uint16_t m_usEvent1;

#ifndef CLIENT_DLL
	bool m_fWasReloading = false;
	float m_flReloadEndTime = 0.0f;
#endif

};
