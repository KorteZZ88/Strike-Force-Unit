#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <utility>

#define WEAPON_SG552 43
#define SG552_WEIGHT 16
#define SG552_MAX_CLIP 30
#define SG552_MAX_SPARE_MAGAZINES 4
#define SG552_DEFAULT_GIVE 30
#define SG552_CLASSNAME weapon_sg552

enum sg552_e { SG552_IDLE, SG552_RELOAD, SG552_DRAW, SG552_SHOOT1, SG552_SHOOT2, SG552_SHOOT3 };

class CSG552WeaponContext : public CBaseWeaponContext
{
public:
	CSG552WeaponContext() = delete;
	explicit CSG552WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 1; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override;
	bool Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
private:
	uint16_t m_usFireEvent;
};
