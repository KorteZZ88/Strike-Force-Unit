#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_MINE_AP 55
// v_landmine.mdl: plant has 36 frames at 25 fps (35 frame intervals).
constexpr float MINE_AP_PLANT_TIME = 35.0f / 25.0f;
class CMineAPWeaponContext : public CBaseWeaponContext
{
public:
 explicit CMineAPWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
 int iItemSlot() override { return 5; }
 int GetItemInfo(ItemInfo* p) const override;
 bool Deploy() override;
 void Holster() override;
 void PrimaryAttack() override;
 void PrimaryAttackReleased() override;
 void SecondaryAttack() override {}
 void Reload() override {}
 void WeaponIdle() override;
};
template<> struct CBaseWeaponContext::AssignedWeaponID<CMineAPWeaponContext> { static constexpr int32_t value = WEAPON_MINE_AP; };
