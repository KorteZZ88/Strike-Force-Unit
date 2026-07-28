/*
server_weapon_layer_impl.cpp - part of server-side weapons predicting implementation
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

#include "server_weapon_layer_impl.h"
#include "gamerules.h"
#include "game.h"
#include "weapons/glock.h"
#include "weapons/glock18.h"
#include "weapons/usp.h"
#include "weapons/colt1911.h"
#include "weapons/python.h"
#include "weapons/shotgun.h"
#include "weapons/mp5.h"
#include "weapons/m4.h"
#include "weapons/m24.h"
#include "weapons/ak47.h"
#include "weapons/m60.h"

namespace
{
constexpr float BULLET_PENETRATION_EPSILON = 0.01f;
constexpr float BULLET_PENETRATION_EXIT_OFFSET = 0.1f;
constexpr float BULLET_PENETRATION_PROBE_MARGIN = 1.0f;
constexpr float BULLET_PENETRATION_THICKNESS_TOLERANCE = 0.03125f;
constexpr float BRUSH_ENTITY_COLLISION_PADDING = 2.0f;

float GetBulletPenetrationDepth(CBasePlayerWeapon *weapon, int bulletType)
{
	if (weapon && weapon->iWeaponID() == WEAPON_M60)
		return 40.0f;
	switch (bulletType)
	{
	case BULLET_PLAYER_9MM:
	case BULLET_PLAYER_45ACP:
	case BULLET_PLAYER_MP5:
	case BULLET_PLAYER_BUCKSHOT:
		return 8.0f;
	case BULLET_PLAYER_556:
		return 16.0f;
	case BULLET_PLAYER_762:
	case BULLET_PLAYER_762X39:
		return 24.0f;
	default:
		return 0.0f;
	}
}

float GetBulletDamage(CBasePlayerWeapon *weapon, int bulletType, int damage)
{
	if (weapon)
	{
		switch (weapon->iWeaponID())
		{
		case WEAPON_GLOCK18: return GetSkillCvar((char*)"sk_plr_glock18_bullet");
		case WEAPON_BERETTA: return GetSkillCvar((char*)"sk_plr_beretta_bullet");
		case WEAPON_USP:
		{
			CUSPWeaponContext *context = dynamic_cast<CUSPWeaponContext *>(weapon->m_pWeaponContext.get());
			return GetSkillCvar((char *)(context && context->IsSilenced()
				? "sk_plr_usp_silenced_bullet" : "sk_plr_usp_bullet"));
		}
		case WEAPON_COLT1911: return GetSkillCvar((char*)"sk_plr_1911_bullet");
		case WEAPON_RBULL: return GetSkillCvar((char*)"sk_plr_rbull_bullet");
		case WEAPON_SHOTGUN: return 20.0f;
		case WEAPON_MP5: return 26.0f;
		case WEAPON_M4:
		{
			CM4WeaponContext *context = dynamic_cast<CM4WeaponContext *>(weapon->m_pWeaponContext.get());
			return GetSkillCvar((char *)(context && context->IsSilenced()
				? "sk_plr_m4_silenced_bullet" : "sk_plr_m4_bullet"));
		}
		case WEAPON_M24: return GetSkillCvar((char*)"sk_plr_m24_bullet");
		case WEAPON_AK47: return GetSkillCvar((char*)"sk_plr_ak47_bullet");
		case WEAPON_M60: return GetSkillCvar((char*)"sk_plr_m60_bullet");
		default: break;
		}
	}

	if (damage)
		return damage;

	switch (bulletType)
	{
	case BULLET_PLAYER_762:
		return gSkillData.plrDmgM24;
	case BULLET_PLAYER_MP5:
		return gSkillData.plrDmgMP5;
	case BULLET_PLAYER_556:
		return gSkillData.plrDmgM4;
	case BULLET_PLAYER_BUCKSHOT:
		return gSkillData.plrDmgBuckshot;
	case BULLET_PLAYER_RBULL:
		return gSkillData.plrDmgRBull;
	case BULLET_PLAYER_45ACP:
		return gSkillData.plrDmg45ACP;
	case BULLET_NONE:
		return 50.0f;
	default:
	case BULLET_PLAYER_9MM:
		return gSkillData.plrDmg9MM;
	}
}

float GetBulletRangeModifier(CBasePlayerWeapon *weapon)
{
	if (!weapon)
		return 1.0f;

	switch (weapon->iWeaponID())
	{
	case WEAPON_GLOCK18: return 0.75f;
	case WEAPON_BERETTA: return 0.75f;
	case WEAPON_USP: return 0.79f;
	case WEAPON_COLT1911: return 0.79f;
	case WEAPON_RBULL: return 0.81f;  // Desert Eagle
	case WEAPON_MP5: return 0.84f;
	case WEAPON_M4:
	{
		CM4WeaponContext *context = dynamic_cast<CM4WeaponContext *>(weapon->m_pWeaponContext.get());
		return context && context->IsSilenced() ? 0.95f : 0.97f;
	}
	case WEAPON_M24: return 0.98f; // Scout
	case WEAPON_AK47: return 0.98f;
	case WEAPON_M60: return 0.96f;
	default: return 1.0f;
	}
}
}

CServerWeaponLayerImpl::CServerWeaponLayerImpl(CBasePlayerWeapon *weaponEntity) :
	m_pWeapon(weaponEntity)
{
}

int CServerWeaponLayerImpl::GetWeaponBodygroup()
{
	return m_pWeapon->pev->body;
}

void CServerWeaponLayerImpl::SetWeaponBodygroup(int value)
{
	m_pWeapon->pev->body = value;
}

Vector CServerWeaponLayerImpl::GetGunPosition()
{
	return m_pWeapon->m_pPlayer->GetAbsOrigin() + m_pWeapon->m_pPlayer->pev->view_ofs;
}

matrix3x3 CServerWeaponLayerImpl::GetCameraOrientation()
{
	CBasePlayer *player = m_pWeapon->m_pPlayer;
	return matrix3x3(player->pev->v_angle + player->pev->punchangle);
}

Vector CServerWeaponLayerImpl::GetViewAngles()
{
	return m_pWeapon->m_pPlayer->pev->v_angle;
}

Vector CServerWeaponLayerImpl::GetAutoaimVector(float delta)
{
	CBasePlayer *player = m_pWeapon->m_pPlayer;

	if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors( player->pev->v_angle + player->pev->punchangle );
		return gpGlobals->v_forward;
	}

	Vector vecSrc = GetGunPosition( );
	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (1 || g_iSkillLevel == SKILL_MEDIUM)
	{
		player->m_vecAutoAim = Vector( 0, 0, 0 );
		// flDelta *= 0.5;
	}

	BOOL m_fOldTargeting = player->m_fOnTarget;
	Vector angles = player->AutoaimDeflection(vecSrc, flDist, delta );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		player->m_fOnTarget = 0;
	else if (m_fOldTargeting != player->m_fOnTarget)
	{
		player->m_pActiveItem->UpdateItemInfo( );
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (0 || g_iSkillLevel == SKILL_EASY)
	{
		player->m_vecAutoAim = player->m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		player->m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if ( g_psv_aim->value != 0 )
	{
		if ( player->m_vecAutoAim.x != player->m_lastx ||
			 player->m_vecAutoAim.y != player->m_lasty )
		{
			SET_CROSSHAIRANGLE( player->edict(), -player->m_vecAutoAim.x, player->m_vecAutoAim.y );
			
			player->m_lastx = player->m_vecAutoAim.x;
			player->m_lasty = player->m_vecAutoAim.y;
		}
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors( player->pev->v_angle + player->pev->punchangle + player->m_vecAutoAim );
	return gpGlobals->v_forward;
}

Vector CServerWeaponLayerImpl::FireBullets(int bullets, Vector origin, matrix3x3 orientation, float distance, float spread, int bulletType, uint32_t seed, int damage)
{
	float x, y, z;
	TraceResult tr;
	CBasePlayer *player = m_pWeapon->m_pPlayer;

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for (uint32_t i = 1; i <= bullets; i++)
	{
		// use player's random seed, get circular gaussian spread
		x = m_randomGenerator.GetFloat(seed + i, -0.5f, 0.5f) + m_randomGenerator.GetFloat(seed + 1 + i, -0.5f, 0.5f);
		y = m_randomGenerator.GetFloat(seed + 2 + i, -0.5f, 0.5f) + m_randomGenerator.GetFloat(seed + 3 + i, -0.5f, 0.5f);
		z = x * x + y * y;

		Vector vecDir = orientation.GetForward() +
						x * spread * orientation.GetRight() +
						y * spread * orientation.GetUp();
		Vector vecEnd = origin + vecDir * distance;
		const Vector vecTraceDir = vecDir.Normalize();
		float bulletDamage = GetBulletDamage(m_pWeapon, bulletType, damage);

		SetBits(gpGlobals->trace_flags, FTRACE_MATERIAL_TRACE);
		UTIL_TraceLine(origin, vecEnd, dont_ignore_monsters, ENT(player->pev), &tr);
		ClearBits(gpGlobals->trace_flags, FTRACE_MATERIAL_TRACE);

		// do damage, paint decals
		if (tr.flFraction != 1.0)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			const float penetrationDepth = GetBulletPenetrationDepth(m_pWeapon, bulletType);

			if (penetrationDepth > 0.0f && pEntity && pEntity->IsBSPModel() && !pEntity->pev->takedamage)
			{
				TraceResult exitTrace;
				// The engine expands collision bounds of brush entities by one unit
				// on either side. World geometry does not have this extra padding.
				const float collisionPadding = pEntity != g_pWorld ? BRUSH_ENTITY_COLLISION_PADDING : 0.0f;
				const float incidence = fabsf(DotProduct(vecTraceDir, tr.vecPlaneNormal));
				const float penetrationProbeDepth = (penetrationDepth + collisionPadding) /
					Q_max(incidence, 0.01f);
				const Vector penetrationEnd = tr.vecEndPos + vecTraceDir * (penetrationProbeDepth + BULLET_PENETRATION_PROBE_MARGIN);
				const Vector penetrationStart = tr.vecEndPos + vecTraceDir * BULLET_PENETRATION_EPSILON;

				if (pEntity == g_pWorld)
				{
					// World geometry is part of the world hull rather than a standalone
					// brush model, so it must use the regular world trace.
					UTIL_TraceLine(penetrationEnd, penetrationStart, ignore_monsters,
						ENT(player->pev), &exitTrace);
				}
				else
				{
					// Trace a brush entity itself. A regular world trace can select an
					// adjacent brush instead of the far face of a moving BSP door.
					UTIL_TraceModel(penetrationEnd, penetrationStart, point_hull,
						tr.pHit, &exitTrace);
				}

				if (!exitTrace.fStartSolid && exitTrace.flFraction < 1.0f)
				{
					const float tracedThickness = (exitTrace.vecEndPos - tr.vecEndPos).Length() * incidence;
					const float wallThickness = Q_max(0.0f, tracedThickness - collisionPadding);
					if (wallThickness <= penetrationDepth + BULLET_PENETRATION_THICKNESS_TOLERANCE)
					{
						bulletDamage = Q_max(2.0f, bulletDamage - floorf(wallThickness / 2.0f));

						SetBits(gpGlobals->trace_flags, FTRACE_MATERIAL_TRACE);
						UTIL_TraceLine(exitTrace.vecEndPos + vecTraceDir * BULLET_PENETRATION_EXIT_OFFSET,
							vecEnd, dont_ignore_monsters, ENT(player->pev), &tr);
						ClearBits(gpGlobals->trace_flags, FTRACE_MATERIAL_TRACE);

						pEntity = tr.flFraction != 1.0 ? CBaseEntity::Instance(tr.pHit) : nullptr;
					}
				}
			}

			if (pEntity && tr.flFraction != 1.0)
			{
				const float hitDistance = (tr.vecEndPos - origin).Length();
				if (m_pWeapon->iWeaponID() == WEAPON_SHOTGUN)
					bulletDamage *= Q_max(0.0f, 1.0f - hitDistance / 3000.0f);
				else
					bulletDamage *= powf(GetBulletRangeModifier(m_pWeapon), hitDistance / 500.0f);

				const int damageType = bulletType == BULLET_NONE ? DMG_CLUB : DMG_BULLET;
				const int gibType = bulletType == BULLET_NONE && damage && bulletDamage > 16.0f ?
					DMG_ALWAYSGIB : DMG_NEVERGIB;
				pEntity->TraceAttack(player->pev, bulletDamage, vecDir, &tr, damageType | gibType);

				TEXTURETYPE_PlaySound(&tr, origin, vecEnd, bulletType);
				DecalGunshot(&tr, bulletType, origin, vecEnd);
			}
		}
		// make bullet trails
		UTIL_BubbleTrail( origin, tr.vecEndPos, (distance * tr.flFraction) / 64.0 );
	}

	ApplyMultiDamage(player->pev, player->pev);
	return Vector( x * spread, y * spread, 0.0 );
}

int CServerWeaponLayerImpl::GetPlayerAmmo(int ammoType)
{
	return m_pWeapon->m_pPlayer->m_rgAmmo[ammoType];
}

void CServerWeaponLayerImpl::SetPlayerAmmo(int ammoType, int count)
{
	m_pWeapon->m_pPlayer->m_rgAmmo[ammoType] = count;
}

void CServerWeaponLayerImpl::SetPlayerWeaponAnim(int anim)
{
	m_pWeapon->m_pPlayer->pev->weaponanim = anim;
}

void CServerWeaponLayerImpl::SetPlayerViewmodel(std::string_view model)
{
	m_pWeapon->m_pPlayer->pev->viewmodel = MAKE_STRING(model.data());
}

void CServerWeaponLayerImpl::DisablePlayerViewmodel()
{
	m_pWeapon->m_pPlayer->pev->viewmodel = iStringNull;
}

int CServerWeaponLayerImpl::GetPlayerViewmodel()
{
	return m_pWeapon->m_pPlayer->pev->viewmodel;
}

int CServerWeaponLayerImpl::GetPlayerWaterlevel()
{
	return m_pWeapon->m_pPlayer->pev->waterlevel;
}

bool CServerWeaponLayerImpl::CheckPlayerButtonFlag(int buttonMask)
{
	return FBitSet(m_pWeapon->m_pPlayer->pev->button, buttonMask);
}

void CServerWeaponLayerImpl::ClearPlayerButtonFlag(int buttonMask)
{
	ClearBits(m_pWeapon->m_pPlayer->pev->button, buttonMask);
}

float CServerWeaponLayerImpl::GetPlayerNextAttackTime()
{
	return m_pWeapon->m_pPlayer->m_flNextAttack; 
}

void CServerWeaponLayerImpl::SetPlayerNextAttackTime(float value)
{
	m_pWeapon->m_pPlayer->m_flNextAttack = value;
}

void CServerWeaponLayerImpl::SetPlayerFOV(float value)
{
	m_pWeapon->m_pPlayer->pev->fov = value;
	m_pWeapon->m_pPlayer->m_iFOV = value;
}

float CServerWeaponLayerImpl::GetPlayerFOV()
{
	return m_pWeapon->m_pPlayer->pev->fov;
}

Vector CServerWeaponLayerImpl::GetPlayerVelocity()
{
	return m_pWeapon->m_pPlayer->GetAbsVelocity();
}

void CServerWeaponLayerImpl::SetPlayerVelocity(Vector value)
{
	m_pWeapon->m_pPlayer->SetAbsVelocity(value);
}

int CServerWeaponLayerImpl::PrepareMagazineReload(int magazineType, int ammoType, int capacity, int weaponRounds, bool tactical)
{
	const int selected = m_pWeapon->m_pPlayer->GetFullestMagazine(magazineType);
	if (selected < 0)
		return -1;
	return m_pWeapon->m_pPlayer->m_rgMagazineRounds[selected] + (weaponRounds > 0 ? 1 : 0);
}

int CServerWeaponLayerImpl::CompleteMagazineReload(int magazineType, int ammoType, int capacity, int weaponRounds, bool tactical)
{
	return m_pWeapon->m_pPlayer->CompleteMagazineReload(magazineType, ammoType, capacity, weaponRounds, tactical ? TRUE : FALSE);
}

void CServerWeaponLayerImpl::CancelMagazineReload()
{
}

float CServerWeaponLayerImpl::GetWeaponTimeBase(bool usePredicting)
{
	return usePredicting ? 0.0f : gpGlobals->time;
}

float CServerWeaponLayerImpl::GetTime()
{
	return gpGlobals->time;
}

uint32_t CServerWeaponLayerImpl::GetRandomSeed()
{
	return m_pWeapon->m_pPlayer->random_seed;
}

uint32_t CServerWeaponLayerImpl::GetRandomInt(uint32_t seed, int32_t min, int32_t max)
{
	return m_randomGenerator.GetInteger(seed, min, max);
}

float CServerWeaponLayerImpl::GetRandomFloat(uint32_t seed, float min, float max)
{
	return m_randomGenerator.GetFloat(seed, min, max);
}

uint16_t CServerWeaponLayerImpl::PrecacheEvent(const char *eventName)
{
	return g_engfuncs.pfnPrecacheEvent(1, eventName);
}

void CServerWeaponLayerImpl::PlaybackWeaponEvent(const WeaponEventParams &params)
{
	// this weird division-multiplying by 3 is somehow relatable to stupid quake bug
	// TODO maybe create new features flag in engine to get rid of this hack entirely?
	Vector anglesFixed = params.angles;
	anglesFixed[PITCH] /= 3.f;
	g_engfuncs.pfnPlaybackEvent(static_cast<int>(params.flags), m_pWeapon->m_pPlayer->edict(),
		params.eventindex, params.delay, 
		params.origin, anglesFixed, 
		params.fparam1, params.fparam2, 
		params.iparam1, params.iparam2, 
		params.bparam1, params.bparam2);
}

bool CServerWeaponLayerImpl::ShouldRunFuncs()
{
	return true; // always true, because server do not any kind of subticking for weapons
}

bool CServerWeaponLayerImpl::IsMultiplayer()
{
	// in case gamerules not available at the moment, likely this is singleplayer
	// and we're loading from save-file. therefore return false as default value.
	return g_pGameRules ? g_pGameRules->IsMultiplayer() : false;
}

bool CServerWeaponLayerImpl::ShouldAutoReload()
{
	CBasePlayer *player = m_pWeapon ? m_pWeapon->m_pPlayer : NULL;
	if (!player)
		return true;

	const char *value = g_engfuncs.pfnInfoKeyValue(
		g_engfuncs.pfnGetInfoKeyBuffer(player->edict()), "cl_autoreload");
	return !value || !value[0] || atoi(value) != 0;
}
