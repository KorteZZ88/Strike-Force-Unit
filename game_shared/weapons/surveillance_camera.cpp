#include "surveillance_camera.h"
#include <utility>
// v_camera and v_tablet use the shared sequence layout: idle 0, draw 1.
#ifndef CLIENT_DLL
#include "weapon_surveillance_camera.h"
#include "player.h"
#endif
CSurveillanceCameraWeaponContext::CSurveillanceCameraWeaponContext(std::unique_ptr<IWeaponLayer>&&layer):CBaseWeaponContext(std::move(layer)){m_iId=WEAPON_SURVEILLANCE_CAMERA;m_iDefaultAmmo=1;}
int CSurveillanceCameraWeaponContext::GetItemInfo(ItemInfo*p)const{p->pszName="weapon_camera";p->pszAmmo1="Surveillance Camera";p->iMaxAmmo1=1;p->pszAmmo2=nullptr;p->iMaxAmmo2=-1;p->iMaxClip=WEAPON_NOCLIP;p->iSlot=5;p->iPosition=0;p->iFlags=ITEM_FLAG_SELECTONEMPTY|ITEM_FLAG_LIMITINWORLD;p->iId=m_iId;p->iWeight=-10;return 1;}
bool CSurveillanceCameraWeaponContext::IsUseable(){
#ifndef CLIENT_DLL
	auto*w=static_cast<CSurveillanceCamera*>(m_pLayer->GetWeaponEntity());return m_pLayer->GetPlayerAmmo(PrimaryAmmoIndex())>0||w->HasCameras();
#else
	return true;
#endif
}
bool CSurveillanceCameraWeaponContext::CanDeploy(){return IsUseable();}
bool CSurveillanceCameraWeaponContext::Deploy(){
#ifndef CLIENT_DLL
	auto*w=static_cast<CSurveillanceCamera*>(m_pLayer->GetWeaponEntity());
	const bool deployed=DefaultDeploy("models/weapon/Camera/v_tablet.mdl","models/p_satchel_radio.mdl",CAMERA_DRAW,"hive");
	if(deployed){w->UpdateViewModels();w->UpdateCameraIndicators(true);}
	return deployed;
#endif
	return DefaultDeploy("models/weapon/Camera/v_tablet.mdl","models/p_satchel_radio.mdl",CAMERA_DRAW,"hive");
}
void CSurveillanceCameraWeaponContext::Holster(){
#ifndef CLIENT_DLL
	auto *cameraWeapon=static_cast<CSurveillanceCamera*>(m_pLayer->GetWeaponEntity());
	cameraWeapon->LeaveCameraView();
	cameraWeapon->UpdateCameraIndicators(false);
#endif
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting())+0.5f);
}
void CSurveillanceCameraWeaponContext::PrimaryAttack(){
#ifndef CLIENT_DLL
	auto*w=static_cast<CSurveillanceCamera*>(m_pLayer->GetWeaponEntity());if(m_pLayer->GetPlayerAmmo(PrimaryAmmoIndex())>0){if(!w->PlaceCamera()){m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(0.2f);return;}m_pLayer->SetPlayerAmmo(PrimaryAmmoIndex(),0);w->UpdateViewModels();SendWeaponAnim(CAMERA_DRAW);}else w->ToggleCameraView();
#endif
	m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(0.35f);m_flNextSecondaryAttack=m_pLayer->GetWeaponTimeBase(UsePredicting())+0.35f;
}
void CSurveillanceCameraWeaponContext::SecondaryAttack(){
#ifndef CLIENT_DLL
	static_cast<CSurveillanceCamera*>(m_pLayer->GetWeaponEntity())->SelectNextCamera();
#endif
	m_flNextPrimaryAttack=GetNextPrimaryAttackDelay(0.25f);m_flNextSecondaryAttack=m_pLayer->GetWeaponTimeBase(UsePredicting())+0.25f;
}
void CSurveillanceCameraWeaponContext::WeaponIdle(){
#ifndef CLIENT_DLL
	auto *cameraWeapon=static_cast<CSurveillanceCamera*>(m_pLayer->GetWeaponEntity());
	cameraWeapon->UpdateCameraIndicators(true);
	cameraWeapon->UpdateCameraView();
#endif
	if(m_flTimeWeaponIdle<=m_pLayer->GetWeaponTimeBase(UsePredicting())){SendWeaponAnim(CAMERA_IDLE);m_flTimeWeaponIdle=m_pLayer->GetWeaponTimeBase(UsePredicting())+0.1f;}
}
