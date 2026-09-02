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

#include "beretta.h"
#include <utility>

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

CBerettaWeaponContext::CBerettaWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iDefaultAmmo = BERETTA_DEFAULT_GIVE;
	m_iId = WEAPON_BERETTA;
	m_usFireBeretta = m_pLayer->PrecacheEvent("events/beretta.sc");
}

int CBerettaWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(BERETTA_CLASSNAME);
	p->pszAmmo1 = "9mm_beretta";
	p->iMaxAmmo1 = BERETTA_9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BERETTA_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = BERETTA_WEIGHT;
	return 1;
}

int CBerettaWeaponContext::GetReloadClipSize(int requestedClipSize)
{
	return requestedClipSize;
}

bool CBerettaWeaponContext::Deploy( )
{
	// pev->body = 1;
	return DefaultDeploy( "models/weapon/Beretta/v_beretta.mdl", "models/p_9mmhandgun.mdl", BERETTA_DRAW, "onehanded" );
}



void CBerettaWeaponContext::PrimaryAttack( void )
{
	BerettaFire(GetCs16PistolSpread(Cs16PistolProfile::Elite), ConfigFireInterval(60.0f / 350.0f), TRUE);
}

void CBerettaWeaponContext::BerettaFire( float flSpread , float flCycleTime, bool fUseAutoAim )
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f);
		}

		return;
	}

	m_iClip--;

	SendWeaponAnim(m_iClip != 0 ? BERETTA_SHOOT : BERETTA_SHOOT_EMPTY);

#ifndef CLIENT_DLL
	// player "shoot" animation
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;

	player->SetAnimation( PLAYER_ATTACK1 );
	player->pev->effects = (int)(player->pev->effects) | EF_MUZZLEFLASH;

	// silenced
	if (m_pLayer->GetWeaponBodygroup() == 1)
	{
		player->m_iWeaponVolume = QUIET_GUN_VOLUME;
		player->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		player->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		player->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}
#endif

	Vector vecSrc = m_pLayer->GetGunPosition();
	matrix3x3 aimMatrix = m_pLayer->GetCameraOrientation();

	if (fUseAutoAim) {
		aimMatrix.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_10DEGREES));
	}

	Vector vecDir = m_pLayer->FireBullets(1, vecSrc, aimMatrix, 8192, flSpread, BULLET_PLAYER_9MM, m_pLayer->GetRandomSeed());
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(flCycleTime); //(0.086f);
	
	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usFireBeretta;
	params.delay = 0.0f;
	params.origin = vecSrc;
	params.angles = aimMatrix.GetAngles();
	params.fparam1 = vecDir.x;
	params.fparam2 = vecDir.y;
	params.iparam1 = 0;
	params.iparam2 = 0;
	params.bparam1 = (m_iClip == 0) ? 1 : 0;
	params.bparam2 = 0;

	if (m_pLayer->ShouldRunFuncs()) {
		m_pLayer->PlaybackWeaponEvent(params);
	}

	KickBack(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);

	// PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

#ifndef CLIENT_DLL
	if (!m_iClip && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		// HEV suit - indicate out of ammo condition
		m_pLayer->GetWeaponEntity()->m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.f, 15.f);
	//m_pPlayer->pev->punchangle.x -= 2;
}

void CBerettaWeaponContext::Reload( void )
{
	const int reloadClipSize = m_iClip > 0 ? BERETTA_MAX_CLIP + 1 : BERETTA_MAX_CLIP;
	if (m_iClip >= reloadClipSize)
		return;

	const int iResult = DefaultReload(reloadClipSize, BERETTA_RELOAD, 3.1f);

	if (iResult)
	{
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 190.0f;
#endif
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.0f, 15.0f);
	}
}

void CBerettaWeaponContext::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pLayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;

#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 250.0f;
#endif

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 0.0f, 1.0f);
		
		{
			iAnim = BERETTA_IDLE1;
			m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 49.0 / 16;
		}
		
		SendWeaponAnim( iAnim );
	}
}
