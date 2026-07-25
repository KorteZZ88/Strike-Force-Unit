/*
mp5_fire_event.cpp
Copyright (C) 2025 SNMetamorph

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "mp5_fire_event.h"
#include "game_event_utils.h"
#include "hud.h"
#include "const.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#include "weapons/mp5.h"

CMP5FireEvent::CMP5FireEvent(event_args_t *args) :
	CBaseGameEvent(args)
{
}

void DlightFlash(const Vector &origin, int index);

void CMP5FireEvent::Execute(bool secondary)
{
		HandleShot();
}

void CMP5FireEvent::HandleShot()
{
	DlightFlash(GetOrigin(), GetEntityIndex());

	if (IsEventLocal())
	{
		GameEventUtils::SpawnMuzzleflash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_FIRE1 + gEngfuncs.pfnRandomLong(0,2), 2 );
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
	GameEventUtils::FireBullet(m_arguments->entindex, cameraMatrix, GetOrigin(), GetShootDirection(cameraMatrix), 2);

	const char *soundName = gEngfuncs.pfnRandomLong(0, 1) == 0 ? "weapons/MP-5/mp5-1.wav" : "weapons/MP-5/mp5-2.wav";
	gEngfuncs.pEventAPI->EV_PlaySound( GetEntityIndex(), GetOrigin(), CHAN_WEAPON, soundName, 1.f, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong(0, 15));
}


Vector CMP5FireEvent::GetShootDirection(const matrix3x3 &camera) const
{
	return camera.GetForward() + m_arguments->fparam1 * camera.GetRight() + m_arguments->fparam2 * camera.GetUp();
}
