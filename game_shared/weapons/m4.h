#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_M4			17
#define M4_WEIGHT			15
#define M4_MAX_CLIP			30
#define M4_DEFAULT_AMMO		30
#define M4_DEFAULT_GIVE		30
#define M4_CLASSNAME		weapon_m4

enum m4_e
{
	M4_IDLE = 0,
	M4_SHOOT1,
	M4_SHOOT2,
	M4_SHOOT3,
	M4_RELOAD,
	M4_DRAW,
	M4_ADD_SILENCER,
	M4_UNSIL_IDLE,
	M4_UNSIL_SHOOT1,
	M4_UNSIL_SHOOT2,
	M4_UNSIL_SHOOT3,
	M4_UNSIL_RELOAD,
	M4_UNSIL_DRAW,
	M4_DETACH_SILENCER,
};

class CM4WeaponContext : public CBaseWeaponContext
{
public:
	CM4WeaponContext() = delete;
	~CM4WeaponContext() = default;
	CM4WeaponContext(std::unique_ptr<IWeaponLayer> &&layer);

	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	int SecondaryAmmoIndex() override;
	bool Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
	bool IsSilenced() const { return m_bSilenced; }
	void SetSilenced(bool silenced) { m_bSilenced = silenced; }

	uint16_t m_usEvent1;

private:
	bool m_bSilenced = false;
};
