#include "p229_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/p229.h"

CP229FireEvent::CP229FireEvent(event_args_t *args) : CBaseGameEvent(args) {}
void DlightFlash(const Vector &origin, int index);
void CP229FireEvent::Execute()
{
	DlightFlash(GetOrigin(), GetEntityIndex());
	if (IsEventLocal()) { GameEventUtils::SpawnMuzzleflash(); gEngfuncs.pEventAPI->EV_WeaponAnimation(m_arguments->iparam1, 0); }
	matrix3x3 camera(GetAngles()); GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp(), 1);
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, "weapons/P229/p229-1.wav", GetGunshotVolume(), GetGunshotAttenuation(), 0, PITCH_NORM);
}
