#include "m4_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/m4.h"

CM4FireEvent::CM4FireEvent(event_args_t *args) :
	CBaseGameEvent(args)
{
}

void DlightFlash(const Vector &origin, int index);

void CM4FireEvent::Execute(bool secondary)
{
	if (!secondary)
		HandleShot();
	else
		HandleGrenadeLaunch();
}

void CM4FireEvent::HandleShot()
{
	const bool silenced = m_arguments->bparam2 != 0;
	if (!silenced)
		DlightFlash(GetOrigin(), GetEntityIndex());

	if (IsEventLocal())
	{
		if (!silenced)
			GameEventUtils::SpawnMuzzleflash();
		const int firstShootSequence = silenced ? M4_SHOOT1 : M4_UNSIL_SHOOT1;
		gEngfuncs.pEventAPI->EV_WeaponAnimation(firstShootSequence + gEngfuncs.pfnRandomLong(0, 2), 2);
		// V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}

	matrix3x3 cameraMatrix(GetAngles());
	Vector up = cameraMatrix.GetUp();
	Vector right = cameraMatrix.GetRight();
	Vector forward = cameraMatrix.GetForward();
	int brassModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
	Vector shellVelocity = GetVelocity() + right * gEngfuncs.pfnRandomFloat(-110, -120) + up * gEngfuncs.pfnRandomFloat(100, 150) + forward * 25.0f;
	Vector shellOrigin = GetOrigin() + up * -5.0f + forward * 20.0f + right * -4.0f;

	GameEventUtils::EjectBrass(shellOrigin, GetAngles(), shellVelocity, brassModelIndex, TE_BOUNCE_SHELL);
	GameEventUtils::FireBullet(m_arguments->entindex, cameraMatrix, GetOrigin(),
		GetShootDirection(cameraMatrix), silenced ? 0 : 2);

	const char *soundName = silenced
		? "weapons/M4/m4-sil.wav"
		: (gEngfuncs.pfnRandomLong(0, 1) == 0 ? "weapons/M4/m4-1.wav" : "weapons/M4/m4-2.wav");
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, soundName, 1.f, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}

void CM4FireEvent::HandleGrenadeLaunch()
{
	if (IsEventLocal())
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation(M4_RELOAD, 2);
		// V_PunchAxis( 0, -10 );
	}

	const char *soundName = gEngfuncs.pfnRandomLong(0, 1) == 0 ? "weapons/glauncher.wav" : "weapons/glauncher2.wav";
	gEngfuncs.pEventAPI->EV_PlaySound(GetEntityIndex(), GetOrigin(), CHAN_WEAPON, soundName, 1.f, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}

Vector CM4FireEvent::GetShootDirection(const matrix3x3 &camera) const
{
	return camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp();
}
