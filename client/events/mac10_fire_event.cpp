#include "mac10_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/mac10.h"

void CMac10FireEvent::Execute()
{
	if (IsEventLocal())
	{
		GameEventUtils::SpawnMuzzleflash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(MAC10_FIRE1 + gEngfuncs.pfnRandomLong(0, 2), 2);
	}
	matrix3x3 camera(GetAngles());
	Vector right = camera.GetRight(), up = camera.GetUp(), forward = camera.GetForward();
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	Vector velocity = GetVelocity() + right * gEngfuncs.pfnRandomFloat(-120, -110) + up * gEngfuncs.pfnRandomFloat(100, 150) + forward * 25.0f;
	Vector origin = GetOrigin() + up * -5.0f + forward * 20.0f + right * -4.0f;
	GameEventUtils::EjectBrass(origin, GetAngles(), velocity, shell, TE_BOUNCE_SHELL);
	Vector direction = forward + m_arguments->fparam1 * right + m_arguments->fparam2 * up;
	GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), direction, 2);
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON,
		"weapons/Mac-10/mac10-1.wav", GetGunshotVolume(), GetGunshotAttenuation(), 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}
