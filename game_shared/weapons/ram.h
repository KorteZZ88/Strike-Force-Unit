#pragma once

#include "weapon_context.h"
#include "weapon_layer.h"
#include "crowbar.h"
#include <memory>

#define WEAPON_RAM 53
#define RAM_WEIGHT 0
#define RAM_CLASSNAME weapon_ram

class CRamWeaponContext : public CBaseWeaponContext
{
public:
	CRamWeaponContext( std::unique_ptr<IWeaponLayer> &&layer );

	int iItemSlot() override { return 3; }
	int GetItemInfo( ItemInfo *p ) const override;
	bool Deploy() override;
	void Holster() override;
	void PrimaryAttack() override;

private:
	void PlaybackEvent();
	uint16_t m_usRam;
};

template<>
struct CBaseWeaponContext::AssignedWeaponID<CRamWeaponContext>
{
	static constexpr int32_t value = WEAPON_RAM;
};
