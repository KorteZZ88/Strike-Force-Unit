#include "xm1014_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/xm1014.h"

void DlightFlash(const Vector& origin, int index);

CXM1014FireEvent::CXM1014FireEvent(event_args_t* args) : CBaseGameEvent(args) {}

void CXM1014FireEvent::Execute()
{
	DlightFlash(GetOrigin(), GetEntityIndex());
	if (IsEventLocal())
	{
		GameEventUtils::SpawnMuzzleflash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(m_arguments->iparam1 ? XM1014_FIRE2 : XM1014_FIRE1, 2);
	}

	matrix3x3 camera(GetAngles());
	Vector velocity = GetVelocity() + camera.GetRight() * gEngfuncs.pfnRandomFloat(-140.0f, -120.0f) +
		camera.GetUp() * gEngfuncs.pfnRandomFloat(130.0f, 180.0f) + camera.GetForward() * 25.0f;
	Vector origin = GetOrigin() + camera.GetUp() * -10.0f + camera.GetForward() * 18.0f + camera.GetRight() * -5.0f;
	GameEventUtils::EjectBrass(origin, GetAngles(), velocity,
		gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl"), TE_BOUNCE_SHELL);
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON,
		"weapons/XM1014/xm1014-1.wav", GetGunshotVolume(), GetGunshotAttenuation(), 0,
		94 + gEngfuncs.pfnRandomLong(0, 12));
}
