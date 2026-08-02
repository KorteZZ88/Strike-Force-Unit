#include "m249_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/m249.h"
CM249FireEvent::CM249FireEvent(event_args_t* args) : CBaseGameEvent(args) {}
void DlightFlash(const Vector& origin, int index);
void CM249FireEvent::Execute()
{
	DlightFlash(GetOrigin(), GetEntityIndex()); if (IsEventLocal()) { GameEventUtils::SpawnMuzzleflash(); gEngfuncs.pEventAPI->EV_WeaponAnimation(m_arguments->iparam1 ? M249_SHOOT2 : M249_SHOOT1, 0); }
	matrix3x3 camera(GetAngles()); const Vector up = camera.GetUp(), right = camera.GetRight(), forward = camera.GetForward(); const int shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	GameEventUtils::EjectBrass(GetOrigin() + up * -4.0f + forward * 18.0f + right * -7.0f, GetAngles(), GetVelocity() + right * -120.0f + up * 120.0f + forward * 25.0f, shell, TE_BOUNCE_SHELL);
	GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), camera.GetForward() + m_arguments->fparam1 * right + m_arguments->fparam2 * up, 2);
	const char* sample = m_arguments->iparam1 ? "weapons/M249/m249-2.wav" : "weapons/M249/m249-1.wav"; gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, sample, GetGunshotVolume(), GetGunshotAttenuation(), 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}
