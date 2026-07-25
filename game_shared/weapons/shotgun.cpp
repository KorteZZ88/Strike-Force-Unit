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

#include "shotgun.h"

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

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN			Vector( 0.08716f, 0.04362f, 0.00f ) // 

CShotgunWeaponContext::CShotgunWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_SHOTGUN;
	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;
	m_usSingleFire = m_pLayer->PrecacheEvent("events/shotgun1.sc");
}

int CShotgunWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(SHOTGUN_CLASSNAME);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = SHOTGUN_WEIGHT;
	return 1;
}

bool CShotgunWeaponContext::Deploy()
{
	return DefaultDeploy( "models/weapon/m3/v_m3.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun" );
}

void CShotgunWeaponContext::PrimaryAttack()
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
		if (m_pLayer->ShouldAutoReload())
			Reload();
		PlayEmptySound();
		return;
	}

	m_iClip--;

	Vector vecSrc = m_pLayer->GetGunPosition();
	matrix3x3 cameraTransform = m_pLayer->GetCameraOrientation();
	cameraTransform.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));

	const int32_t bulletsCount = 9;
	const float spreadCoef = 0.0675f; // Counter-Strike 1.6 M3 cone
	Vector spread = m_pLayer->FireBullets(bulletsCount, vecSrc, cameraTransform, 3000, spreadCoef, BULLET_PLAYER_BUCKSHOT, m_pLayer->GetRandomSeed());

	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usSingleFire;
	params.delay = 0.0f;
	params.origin = vecSrc;
	params.angles = cameraTransform.GetAngles();
	params.fparam1 = spread.x;
	params.fparam2 = spread.y;
	params.iparam1 = 0;
	params.iparam2 = 0;
	params.bparam1 = 0;
	params.bparam2 = 0;

	if (m_pLayer->ShouldRunFuncs()) {
		m_pLayer->PlaybackWeaponEvent(params);
	}

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;

	player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = NORMAL_GUN_FLASH;
	player->pev->effects = (int)(player->pev->effects) | EF_MUZZLEFLASH;

	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif

	//m_flPumpTime = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5; // ??? is it correct
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.9f);
	
	if (m_iClip != 0)
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 5.0;
	else
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.75;

#ifndef CLIENT_DLL
	// Firing interrupts the shell-by-shell reload, so restore the carrying speed now.
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 240.0f;
#endif
	m_fInSpecialReload = 0;
}


void CShotgunWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
	if (m_fInSpecialReload == 3)
		return;

	// A second press at 7 rounds chambers one more shell. This is a short
	// top-up with no pump animation; the weapon becomes ready immediately.
	if (m_iClip == SHOTGUN_MAGAZINE_SIZE && m_fInSpecialReload == 0)
	{
		m_fInSpecialReload = 3;
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 180.0f;
#endif
		SendWeaponAnim(SHOTGUN_RELOAD);
		m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.5f);
		m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;
		return;
	}

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		m_fInSpecialReload = 1;
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 180.0f;
#endif
		SendWeaponAnim(SHOTGUN_START_RELOAD);
		m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.6);

		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.6;
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(1.0f);
		m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
			return;

		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

#ifndef CLIENT_DLL
		CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
		if (RANDOM_LONG(0, 1))
			EMIT_SOUND_DYN(ENT(player->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));
		else
			EMIT_SOUND_DYN(ENT(player->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));
#endif

		SendWeaponAnim(SHOTGUN_RELOAD);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) - 1);
		m_fInSpecialReload = 1;
	}
}

void CShotgunWeaponContext::WeaponIdle()
{
	ResetEmptySound( );

	m_pLayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle < m_pLayer->GetWeaponTimeBase(UsePredicting()))
	{
		if (m_fInSpecialReload == 3)
		{
			m_iClip += 1;
			m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) - 1);
			m_fInSpecialReload = 0;
#ifndef CLIENT_DLL
			m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 240.0f;
#endif
			SendWeaponAnim(SHOTGUN_IDLE);
			m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + (60.0 / 12.0);
		}
		else if (m_iClip == 0 && m_fInSpecialReload == 0 &&
			m_pLayer->ShouldAutoReload() && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip < SHOTGUN_MAGAZINE_SIZE && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( SHOTGUN_PUMP );
#ifndef CLIENT_DLL
				// play cocking sound
				CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
				EMIT_SOUND_DYN(ENT(player->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
#endif
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.5;
			}
		}
		else
		{
#ifndef CLIENT_DLL
			m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 240.0f;
#endif
			int iAnim;
			float flRand = m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 0.f, 1.f);
			
			{
				iAnim = SHOTGUN_IDLE;
				m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + (60.0/12.0);// * RANDOM_LONG(2, 5);
			}
			SendWeaponAnim( iAnim );
		}
	}
}
