#pragma once
#include "weapons.h"
class CSFUDoor;
class CStickCamera : public CBasePlayerWeapon { DECLARE_CLASS(CStickCamera,CBasePlayerWeapon); public: CStickCamera();void Spawn()override;void Precache()override;void ToggleCameraView();void LeaveCameraView();void ToggleCameraSide();void ToggleCameraZoom();void UpdateCameraView();bool IsViewingCamera()const{return m_bViewingCamera!=FALSE;} private:CBaseEntity*EnsureViewEntity();CSFUDoor*FindUsableDoor(float &sideSign);BOOL m_bViewingCamera=FALSE;BOOL m_bCameraZoom=FALSE;BOOL m_bLookRight=FALSE;EHANDLE m_hViewEntity;EHANDLE m_hCameraDoor;float m_flDoorSideSign=1.0f;Vector m_vecPlayerViewAngles;Vector m_vecCameraStartAngles;};
