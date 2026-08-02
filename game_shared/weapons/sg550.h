#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_SG550 47
#define SG550_CLASSNAME weapon_sg550
#define SG550_MAX_CLIP 30
#define SG550_DEFAULT_GIVE 30
#define SG550_MAX_SPARE_MAGAZINES 3
#define SG550_WEIGHT 20

enum sg550_e { SG550_IDLE, SG550_SHOOT1, SG550_SHOOT2, SG550_RELOAD, SG550_DRAW };

class CSG550WeaponContext : public CBaseWeaponContext
{
public:
	explicit CSG550WeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 4; }
	int GetItemInfo(ItemInfo* info) const override;
	int GetReloadClipSize(int requestedClipSize) override { return requestedClipSize; }
	bool Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void Holster() override;
	bool IsSemiAutomatic() const override { return true; }

private:
	uint16_t m_usFireEvent = 0;
};
