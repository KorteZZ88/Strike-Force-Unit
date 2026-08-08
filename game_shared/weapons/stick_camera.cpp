#include "stick_camera.h"
#include <utility>
#ifndef CLIENT_DLL
#include "weapon_stick_camera.h"
#endif

CStickCameraWeaponContext::CStickCameraWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer)) { m_iId = WEAPON_STICK_CAMERA; m_iDefaultAmmo = 0; }
int CStickCameraWeaponContext::GetItemInfo(ItemInfo* p) const { p->pszName="weapon_stickcamera";p->pszAmmo1=nullptr;p->iMaxAmmo1=-1;p->pszAmmo2=nullptr;p->iMaxAmmo2=-1;p->iMaxClip=WEAPON_NOCLIP;p->iSlot=5;p->iPosition=1;p->iFlags=ITEM_FLAG_SELECTONEMPTY|ITEM_FLAG_LIMITINWORLD;p->iId=m_iId;p->iWeight=-9;return 1; }
bool CStickCameraWeaponContext::Deploy() { return DefaultDeploy("models/weapon/StickCamera/v_stickcamera.mdl","models/p_shotgun.mdl",STICK_CAMERA_DRAW,"shotgun"); }
void CStickCameraWeaponContext::Holster() {
#ifndef CLIENT_DLL
	static_cast<CStickCamera*>(m_pLayer->GetWeaponEntity())->LeaveCameraView();
#endif
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting())+0.5f);
}
void CStickCameraWeaponContext::PrimaryAttack() {
#ifndef CLIENT_DLL
	static_cast<CStickCamera*>(m_pLayer->GetWeaponEntity())->ToggleCameraView();
#endif
	m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(0.3f);m_flNextSecondaryAttack=m_pLayer->GetWeaponTimeBase(UsePredicting())+0.3f;
}
void CStickCameraWeaponContext::SecondaryAttack() {
#ifndef CLIENT_DLL
	static_cast<CStickCamera*>(m_pLayer->GetWeaponEntity())->ToggleCameraSide();
#endif
	m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(0.25f);m_flNextSecondaryAttack=m_pLayer->GetWeaponTimeBase(UsePredicting())+0.25f;
}
void CStickCameraWeaponContext::WeaponIdle() {
#ifndef CLIENT_DLL
	static_cast<CStickCamera*>(m_pLayer->GetWeaponEntity())->UpdateCameraView();
#endif
	if(m_flTimeWeaponIdle<=m_pLayer->GetWeaponTimeBase(UsePredicting())){SendWeaponAnim(STICK_CAMERA_IDLE);m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+0.1f;}
}
