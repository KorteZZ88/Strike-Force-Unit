#include "colt1911_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/usp.h"

CColt1911FireEvent::CColt1911FireEvent(event_args_t *args) : CBaseGameEvent(args) {}

void CColt1911FireEvent::Execute()
{
	if (IsEventLocal())
		gEngfuncs.pEventAPI->EV_WeaponAnimation(m_arguments->bparam1 ? USP_SHOOT_EMPTY : USP_SHOOT1, 0);

	matrix3x3 camera(GetAngles());
	GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(),
		camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp(), 0);

	// This deliberately runs for both local and remote entities, just like the
	// other firearm events, so every client hears the shooter's Colt.
	const char *sound = gEngfuncs.pfnRandomLong(0, 1) == 0 ?
		"weapons/1911/1911-1.wav" : "weapons/1911/1911-2.wav";
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON,
		sound, GetGunshotVolume(), ATTN_NORM, 0, PITCH_NORM);
}
