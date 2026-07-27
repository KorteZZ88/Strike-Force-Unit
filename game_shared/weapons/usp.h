#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_USP 23
#define USP_CLASSNAME weapon_usp
#define USP_MAX_CLIP 12
#define USP_DEFAULT_GIVE 12
#define USP_MAX_SPARE_MAGAZINES 4
#define USP_WEIGHT 10

enum usp_e
{
	USP_IDLE = 0, USP_SHOOT1, USP_SHOOT2, USP_SHOOT3, USP_SHOOT_EMPTY,
	USP_RELOAD, USP_DRAW, USP_ATTACH_SILENCER, USP_UNSIL_IDLE,
	USP_UNSIL_SHOOT1, USP_UNSIL_SHOOT2, USP_UNSIL_SHOOT3,
	USP_UNSIL_SHOOT_EMPTY, USP_UNSIL_RELOAD, USP_UNSIL_DRAW,
	USP_DETACH_SILENCER
};

class CUSPWeaponContext : public CBaseWeaponContext
{
public:
	explicit CUSPWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 2; }
	int GetItemInfo(ItemInfo *p) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	bool IsSemiAutomatic() const override { return true; }
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	bool IsSilenced() const { return m_bSilenced; }
	void SetSilenced(bool silenced) { m_bSilenced = silenced; }
private:
	void USPFire(float spread);
	bool m_bSilenced = false;
	float m_flAccuracy = 0.92f;
	float m_flLastFire = 0.0f;
	uint16_t m_usFireUSP = 0;
};
