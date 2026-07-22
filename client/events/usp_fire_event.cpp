#include "usp_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/usp.h"

CUSPFireEvent::CUSPFireEvent(event_args_t *args) : CBaseGameEvent(args) {}
void CUSPFireEvent::Execute()
{
	const bool silenced = m_arguments->bparam2 != 0;
	if (IsEventLocal()) { if (!silenced) GameEventUtils::SpawnMuzzleflash(); gEngfuncs.pEventAPI->EV_WeaponAnimation(silenced ? (m_arguments->bparam1 ? USP_SHOOT_EMPTY : USP_SHOOT1) : (m_arguments->bparam1 ? USP_UNSIL_SHOOT_EMPTY : USP_UNSIL_SHOOT1), 0); }
	matrix3x3 camera(GetAngles()); GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp(), 1);
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, silenced ? "weapons/USP/usp1.wav" : "weapons/USP/usp_unsil-1.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM);
}
