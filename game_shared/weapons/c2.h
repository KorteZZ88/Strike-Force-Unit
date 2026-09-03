#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_C2 54
#define C2_CLASSNAME weapon_c2

class CC2WeaponContext : public CBaseWeaponContext
{
public:
	CC2WeaponContext( std::unique_ptr<IWeaponLayer> &&layer );
	int iItemSlot() override { return 4; }
	int GetItemInfo( ItemInfo *p ) const override;
	bool Deploy() override;
	bool CanDeploy() override;
	bool IsUseable() override;
	void Holster() override;
	void PrimaryAttack() override;
	void WeaponIdle() override;
	int m_chargeReady;
	float m_flPlaceStart;
};

template<> struct CBaseWeaponContext::AssignedWeaponID<CC2WeaponContext>
{ static constexpr int32_t value = WEAPON_C2; };
