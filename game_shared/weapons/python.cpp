/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "python.h"

#ifdef CLIENT_DLL
#else
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#endif

// check VECTOR_CONE_1DEGREES macro
#define RBULL_STANDING_SPREAD 0.00873f
#define RBULL_MOVING_SPREAD 0.025f

namespace
{
constexpr float RBULL_FIRE_INTERVAL = 60.0f / 70.0f;
}

CRBullWeaponContext::CRBullWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_RBULL;
	m_iDefaultAmmo = RBULL_DEFAULT_GIVE;
	m_fInZoom = false;
	m_usFireRBull = m_pLayer->PrecacheEvent("events/rbull.sc");
}

int CRBullWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(RBULL_CLASSNAME);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RBULL_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 6;
	p->iId = m_iId;
	p->iWeight = RBULL_WEIGHT;
	return 1;
}

bool CRBullWeaponContext::Deploy()
{
	// Strike Force Unit's RBull does not use laser-sight geometry.
	m_pLayer->SetWeaponBodygroup(0);

	return DefaultDeploy("models/weapon/RBull/v_rbull.mdl", "models/weapon/RBull/p_rbull.mdl", RBULL_DRAW, "python");
}

void CRBullWeaponContext::Holster()
{
	m_fInReload = FALSE; // cancel any reload in progress.

	if (m_fInZoom) {
		SecondaryAttack();
	}

	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}

void CRBullWeaponContext::SecondaryAttack()
{
	// Raging Bull has no alternate fire in Strike Force Unit.
	m_fInZoom = false;
}

void CRBullWeaponContext::PrimaryAttack()
{
	// don't fire underwater
	if (m_pLayer->GetPlayerWaterlevel() == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f);
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
		{
			Reload();
		}
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f);
		}
		return;
	}

	m_iClip--;

	Vector vecSrc = m_pLayer->GetGunPosition();
	matrix3x3 cameraTransform = m_pLayer->GetCameraOrientation();
	cameraTransform.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_10DEGREES));
	const bool moving = m_pLayer->GetPlayerVelocity().Length2D() > 0.0f;
	const float bulletSpread = moving ? RBULL_MOVING_SPREAD : RBULL_STANDING_SPREAD;
	Vector spread = m_pLayer->FireBullets(1, vecSrc, cameraTransform, 8192, bulletSpread, BULLET_PLAYER_RBULL, m_pLayer->GetRandomSeed());

	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usFireRBull;
	params.delay = 0.0f;
	params.origin = vecSrc;
	params.angles = cameraTransform.GetAngles();
	params.fparam1 = spread.x;
	params.fparam2 = spread.y;
	params.iparam1 = 0;
	params.iparam2 = 0;
	params.bparam1 = (m_iClip == 0) ? 1 : 0;
	params.bparam2 = 0;

	if (m_pLayer->ShouldRunFuncs()) {
		m_pLayer->PlaybackWeaponEvent(params);
	}

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;

	player->SetAnimation(PLAYER_ATTACK1);
	player->pev->effects = (int)(player->pev->effects) | EF_MUZZLEFLASH;
	player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	// m_pPlayer->pev->punchangle.x -= 10;
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(RBULL_FIRE_INTERVAL);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}

void CRBullWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) < 1)
		return;

	if (m_pLayer->GetPlayerFOV() != 0.0f)
	{
		m_pLayer->SetPlayerFOV(0.0f); // 0 means reset to default fov
		m_fInZoom = false;
	}

	if (DefaultReload(RBULL_MAX_CLIP, RBULL_RELOAD, 94.0f / 34.0f, 0))
	{
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 190.0f;
#endif
	}
}

void CRBullWeaponContext::WeaponIdle()
{
	ResetEmptySound();

	m_pLayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;

#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 250.0f;
#endif

	// RBull contains a single two-frame idle sequence.
	m_flTimeWeaponIdle = 2.0f;
	SendWeaponAnim(RBULL_IDLE1, 0);
}
