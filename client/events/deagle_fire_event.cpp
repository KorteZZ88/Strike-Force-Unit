#include "deagle_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "event_api.h"
#include "event_args.h"

CDeagleFireEvent::CDeagleFireEvent(event_args_t *args) : CBaseGameEvent(args) {}
void DlightFlash(const Vector &origin, int index);

void CDeagleFireEvent::Execute()
{
	DlightFlash(GetOrigin(), GetEntityIndex());
	if (IsEventLocal())
	{
		GameEventUtils::SpawnMuzzleflash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(m_arguments->iparam1, 0);
	}
	matrix3x3 camera(GetAngles());
	Vector direction = camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp();
	GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), direction, 1);
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, "weapons/DEagle/deagle-1.wav", GetGunshotVolume(), GetGunshotAttenuation(), 0, PITCH_NORM);
}
