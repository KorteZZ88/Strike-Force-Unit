#include "aug_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/aug.h"
CAUGFireEvent::CAUGFireEvent(event_args_t* args) : CBaseGameEvent(args) {}
void DlightFlash(const Vector& origin, int index);
void CAUGFireEvent::Execute()
{
	DlightFlash(GetOrigin(), GetEntityIndex());
	if (IsEventLocal()) { GameEventUtils::SpawnMuzzleflash(); gEngfuncs.pEventAPI->EV_WeaponAnimation(AUG_SHOOT1 + gEngfuncs.pfnRandomLong(0, 2), 0); }
	matrix3x3 camera(GetAngles()); const Vector up = camera.GetUp(), right = camera.GetRight(), forward = camera.GetForward();
	const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	const Vector velocity = GetVelocity() + right * gEngfuncs.pfnRandomFloat(-120, -110) + up * gEngfuncs.pfnRandomFloat(100, 150) + forward * 25.0f;
	const Vector origin = GetOrigin() + up * -5.0f + forward * 20.0f + right * -4.0f;
	GameEventUtils::EjectBrass(origin, GetAngles(), velocity, shell, TE_BOUNCE_SHELL);
	GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), camera.GetForward() + m_arguments->fparam1 * right + m_arguments->fparam2 * up, 2);
	const char* sound = gEngfuncs.pfnRandomLong(0, 1) ? "weapons/AUG/aug-1.wav" : "weapons/AUG/aug-2.wav";
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, sound, GetGunshotVolume(), GetGunshotAttenuation(), 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}
