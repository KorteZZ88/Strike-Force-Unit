#include "mp5sd_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/mp5sd.h"

CMP5SDFireEvent::CMP5SDFireEvent(event_args_t *args) : CBaseGameEvent(args) {}

void CMP5SDFireEvent::Execute()
{
	if (IsEventLocal())
		gEngfuncs.pEventAPI->EV_WeaponAnimation(MP5SD_FIRE1 + gEngfuncs.pfnRandomLong(0, 2), 2);

	matrix3x3 cameraMatrix(GetAngles());
	Vector up = cameraMatrix.GetUp();
	Vector right = cameraMatrix.GetRight();
	Vector forward = cameraMatrix.GetForward();
	int brassModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	Vector shellVelocity = GetVelocity() + right * gEngfuncs.pfnRandomFloat(-110, -120) + up * gEngfuncs.pfnRandomFloat(100, 150) + forward * 25.0f;
	Vector shellOrigin = GetOrigin() + up * -5.0f + forward * 20.0f + right * -4.0f;
	GameEventUtils::EjectBrass(shellOrigin, GetAngles(), shellVelocity, brassModelIndex, TE_BOUNCE_SHELL);
	GameEventUtils::FireBullet(m_arguments->entindex, cameraMatrix, GetOrigin(), GetShootDirection(cameraMatrix), 0);
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON,
		"weapons/MP-5SD/mp5-1.wav", GetGunshotVolume(), ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}

Vector CMP5SDFireEvent::GetShootDirection(const matrix3x3 &camera) const
{
	return camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp();
}
