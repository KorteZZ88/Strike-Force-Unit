#pragma once
#include "weapons.h"
class CStickCamera : public CBasePlayerWeapon { DECLARE_CLASS(CStickCamera,CBasePlayerWeapon); public: CStickCamera();void Spawn()override;void Precache()override;void ToggleCameraView();void LeaveCameraView();void ToggleCameraSide();void ToggleCameraZoom();void UpdateCameraView();bool IsViewingCamera()const{return m_bViewingCamera!=FALSE;} private:CBaseEntity*EnsureViewEntity();BOOL m_bViewingCamera=FALSE;BOOL m_bCameraZoom=FALSE;BOOL m_bLookRight=FALSE;EHANDLE m_hViewEntity;Vector m_vecPlayerViewAngles;Vector m_vecCameraStartAngles;};
