#include "g3sg1_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/g3sg1.h"
CG3SG1FireEvent::CG3SG1FireEvent(event_args_t* args) : CBaseGameEvent(args) {}
void DlightFlash(const Vector& origin, int index);
void CG3SG1FireEvent::Execute()
{
	DlightFlash(GetOrigin(), GetEntityIndex());
	if (IsEventLocal()) { GameEventUtils::SpawnMuzzleflash(); gEngfuncs.pEventAPI->EV_WeaponAnimation(m_arguments->iparam1, 0); }
	matrix3x3 camera(GetAngles()); const Vector up=camera.GetUp(), right=camera.GetRight(), forward=camera.GetForward();
	const int shell=gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	GameEventUtils::EjectBrass(GetOrigin()+up*-5.0f+forward*20.0f+right*-4.0f, GetAngles(), GetVelocity()+right*-115.0f+up*125.0f+forward*25.0f, shell, TE_BOUNCE_SHELL);
	GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), forward+m_arguments->fparam1*right+m_arguments->fparam2*up, 2);
	const char* sound=(m_arguments->iparam1==G3SG1_SHOOT1)?"weapons/G3SG1/g3sg1-1.wav":"weapons/G3SG1/g3sg1-2.wav";
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, sound, GetGunshotVolume(), GetGunshotAttenuation(), 0, 94+gEngfuncs.pfnRandomLong(0,15));
}
