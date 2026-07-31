#include "bizon_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/bizon.h"

CBizonFireEvent::CBizonFireEvent(event_args_t *args) : CBaseGameEvent(args) {}

void CBizonFireEvent::Execute()
{
	if (IsEventLocal())
		gEngfuncs.pEventAPI->EV_WeaponAnimation(BIZON_FIRE1 + gEngfuncs.pfnRandomLong(0, 2), 2);
	matrix3x3 camera(GetAngles());
	Vector up = camera.GetUp(), right = camera.GetRight(), forward = camera.GetForward();
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	Vector velocity = GetVelocity() + right * gEngfuncs.pfnRandomFloat(-110, -120) + up * gEngfuncs.pfnRandomFloat(100, 150) + forward * 25.0f;
	Vector origin = GetOrigin() + up * -5.0f + forward * 20.0f + right * -4.0f;
	GameEventUtils::EjectBrass(origin, GetAngles(), velocity, shell, TE_BOUNCE_SHELL);
	GameEventUtils::FireBullet(m_arguments->entindex, camera, GetOrigin(), GetShootDirection(camera), 0);
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON,
		"weapons/Bizon/bizon-1.wav", GetGunshotVolume(), ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}

Vector CBizonFireEvent::GetShootDirection(const matrix3x3 &camera) const
{
	return camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp();
}
