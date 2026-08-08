#pragma once
#include "weapon_context.h"
#include "weapon_layer.h"
#include <memory>

#define WEAPON_STICK_CAMERA 52
#define STICK_CAMERA_CLASSNAME weapon_stickcamera
enum stick_camera_e { STICK_CAMERA_IDLE = 0, STICK_CAMERA_DRAW = 1 };

class CStickCameraWeaponContext : public CBaseWeaponContext {
public:
	CStickCameraWeaponContext() = delete;
	explicit CStickCameraWeaponContext(std::unique_ptr<IWeaponLayer>&& layer);
	int iItemSlot() override { return 5; }
	int GetItemInfo(ItemInfo* info) const override;
	bool Deploy() override;
	void Holster() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponIdle() override;
};
template<> struct CBaseWeaponContext::AssignedWeaponID<CStickCameraWeaponContext> { static constexpr int32_t value = WEAPON_STICK_CAMERA; };
