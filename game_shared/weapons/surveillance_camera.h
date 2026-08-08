#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>
#define WEAPON_SURVEILLANCE_CAMERA 51
#define SURVEILLANCE_CAMERA_CLASSNAME weapon_camera
enum surveillance_camera_e { CAMERA_IDLE=0, CAMERA_DRAW=1 };
class CSurveillanceCameraWeaponContext : public CBaseWeaponContext {
public:
	CSurveillanceCameraWeaponContext()=delete; explicit CSurveillanceCameraWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override{return 5;} int GetItemInfo(ItemInfo*p)const override; bool IsUseable()override; bool CanDeploy()override;
	bool Deploy()override; void Holster()override; void PrimaryAttack()override; void SecondaryAttack()override; void WeaponIdle()override;
};
template<> struct CBaseWeaponContext::AssignedWeaponID<CSurveillanceCameraWeaponContext>{static constexpr int32_t value=WEAPON_SURVEILLANCE_CAMERA;};
