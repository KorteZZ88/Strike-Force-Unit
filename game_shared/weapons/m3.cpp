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

#include "m3.h"

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

namespace
{
	constexpr float M3_CENTER_PELLET_SPREAD = 0.001f;
	constexpr float M3_NEAR_CENTER_PELLET_SPREAD = 0.0015f;
	constexpr float M3_UPPER_PELLETS_SPREAD = 0.025f;
	constexpr float M3_UPPER_PELLETS_OFFSET = 0.018f;
	constexpr float M3_OUTER_PELLETS_SPREAD = 0.05f;
}

CM3WeaponContext::CM3WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_M3;
	m_iDefaultAmmo = M3_DEFAULT_GIVE;
	m_usSingleFire = m_pLayer->PrecacheEvent("events/m3.sc");
}

int CM3WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(M3_CLASSNAME);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M3_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId;
	p->iWeight = M3_WEIGHT;
	return 1;
}

bool CM3WeaponContext::Deploy()
{
	return DefaultDeploy( "models/weapon/m3/v_m3.mdl", "models/p_shotgun.mdl", M3_DRAW, "shotgun" );
}

void CM3WeaponContext::PrimaryAttack()
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

	const uint32_t randomSeed = m_pLayer->GetRandomSeed();

	// Give the pump shotgun a stable core without turning the whole blast into a rifle:
	// one AK-accurate pellet, one immediately around it, three in the upper portion
	// of the pattern, and four pellets using the regular 0.05 cone.
	Vector spread = m_pLayer->FireBullets(1, vecSrc, cameraTransform, 3000,
		M3_CENTER_PELLET_SPREAD, BULLET_PLAYER_BUCKSHOT, randomSeed);
	m_pLayer->FireBullets(1, vecSrc, cameraTransform, 3000,
		M3_NEAR_CENTER_PELLET_SPREAD, BULLET_PLAYER_BUCKSHOT, randomSeed + 11);

	matrix3x3 upperPelletsTransform = cameraTransform;
	upperPelletsTransform.SetForward((cameraTransform.GetForward() +
		cameraTransform.GetUp() * M3_UPPER_PELLETS_OFFSET).Normalize());
	m_pLayer->FireBullets(3, vecSrc, upperPelletsTransform, 3000,
		M3_UPPER_PELLETS_SPREAD, BULLET_PLAYER_BUCKSHOT, randomSeed + 23);

	m_pLayer->FireBullets(4, vecSrc, cameraTransform, 3000,
		M3_OUTER_PELLETS_SPREAD, BULLET_PLAYER_BUCKSHOT, randomSeed + 47);
	const bool onGround = IsPlayerOnGround();
	KickBack(static_cast<float>(m_pLayer->GetRandomInt(m_pLayer->GetRandomSeed() + 1,
		onGround ? 4 : 8, onGround ? 6 : 11)), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);

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


void CM3WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0 || m_iClip == M3_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;
	if (m_fInSpecialReload == 3)
		return;

	// A second press at 7 rounds chambers one more shell. This is a short
	// top-up with no pump animation; the weapon becomes ready immediately.
	if (m_iClip == M3_MAGAZINE_SIZE && m_fInSpecialReload == 0)
	{
		m_fInSpecialReload = 3;
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 180.0f;
#endif
		SendWeaponAnim(M3_RELOAD);
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
		SendWeaponAnim(M3_START_RELOAD);
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

		SendWeaponAnim(M3_RELOAD);
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

void CM3WeaponContext::WeaponIdle()
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
			SendWeaponAnim(M3_IDLE);
			m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + (60.0 / 12.0);
		}
		else if (m_iClip == 0 && m_fInSpecialReload == 0 &&
			m_pLayer->ShouldAutoReload() && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip < M3_MAGAZINE_SIZE && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( M3_PUMP );
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
				iAnim = M3_IDLE;
				m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + (60.0/12.0);// * RANDOM_LONG(2, 5);
			}
			SendWeaponAnim( iAnim );
		}
	}
}
