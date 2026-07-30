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

#include "mp5a3.h"

#ifdef CLIENT_DLL
#else
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "ggrenade.h"
#endif

CMP5A3WeaponContext::CMP5A3WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_MP5A3;
	m_iDefaultAmmo = MP5A3_DEFAULT_GIVE;
	m_usEvent1 = m_pLayer->PrecacheEvent("events/mp5a3.sc");
}

int CMP5A3WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(MP5A3_CLASSNAME);
	p->pszAmmo1 = "9mm_mp5";
	p->iMaxAmmo1 = MP5_9MM_MAX_CARRY;
	p->iMaxClip = MP5A3_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = MP5A3_WEIGHT;
	return 1;
}


int CMP5A3WeaponContext::GetReloadClipSize(int requestedClipSize)
{
	return requestedClipSize;
}


bool CMP5A3WeaponContext::Deploy()
{	
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 250; // Замедление игрока 
#endif
	return DefaultDeploy( "models/weapon/mp5/v_mp5.mdl", "models/p_9mmAR.mdl", MP5A3_DEPLOY, "mp5" );
}

void CMP5A3WeaponContext::PrimaryAttack()
{
	// don't fire underwater
	if (m_pLayer->GetPlayerWaterlevel() == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.086f);
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.086f);
		return;
	}

	m_iClip--;

	Vector vecSrc = m_pLayer->GetGunPosition();
	matrix3x3 cameraTransform = m_pLayer->GetCameraOrientation();
	cameraTransform.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	Vector spread = VECTOR_CONE_2DEGREES;
	Vector vecDir = m_pLayer->FireBullets(1, vecSrc, cameraTransform, 8192, spread.x, BULLET_PLAYER_MP5, m_pLayer->GetRandomSeed());
	KickBack(0.32f, 0.10f, 0.045f, 0.014f, 2.6f, 0.9f, 3);

	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usEvent1;
	params.delay = 0.0f;
	params.origin = vecSrc;
	params.angles = cameraTransform.GetAngles();
	params.fparam1 = vecDir.x;
	params.fparam2 = vecDir.y;
	params.iparam1 = 0;
	params.iparam2 = 0;
	params.bparam1 = 0;
	params.bparam2 = 0;

	if (m_pLayer->ShouldRunFuncs()) {
		m_pLayer->PlaybackWeaponEvent(params);
	}

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	player->m_iWeaponFlash = NORMAL_GUN_FLASH;
	player->pev->effects = (int)(player->pev->effects) | EF_MUZZLEFLASH;
	player->SetAnimation(PLAYER_ATTACK1);

	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.07f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}


void CMP5A3WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;

	// m_iClip is the total number of shots ready to fire. A remaining round
	// means one is chambered: a new 30-round magazine then gives 30+1.
	const int reloadClipSize = m_iClip > 0 ? MP5A3_MAX_CLIP + 1 : MP5A3_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;

	if (DefaultReload(reloadClipSize, MP5A3_RELOAD, 3.0f))
	{
#ifndef CLIENT_DLL
		CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
		player->pev->maxspeed = 190; // Замедление игрока при перезарядке
#endif
	}
}

void CMP5A3WeaponContext::WeaponIdle()
{
	ResetEmptySound();
	m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 250; // Замедление игрока 
#endif

	SendWeaponAnim(m_pLayer->GetRandomInt(m_pLayer->GetRandomSeed(), 0, 1) == 0 ? MP5A3_LONGIDLE : MP5A3_IDLE1);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
}

void CMP5A3WeaponContext::Holster()
{
	m_fInReload = FALSE;
	m_iReloadClipSize = 0; // cancel any reload in progress.
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->pev->maxspeed = 0; //Сброс скорости игрока
#endif

	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
	SendWeaponAnim(MP5A3_FIRE1);
}
