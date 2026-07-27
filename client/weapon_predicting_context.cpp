/*
weapon_predicting_context.cpp - part of client-side weapons predicting implementation
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

#include "weapon_predicting_context.h"
#include "client_weapon_layer_impl.h"
#include "hud.h"
#include "utils.h"
#include "weapons/glock.h"
#include "weapons/glock18.h"
#include "weapons/crossbow.h"
#include "weapons/python.h"
#include "weapons/usp.h"
#include "weapons/mp5.h"
#include "weapons/shotgun.h"
#include "weapons/crowbar.h"
#include "weapons/wrench.h"
#include "weapons/tripmine.h"
#include "weapons/snark.h"
#include "weapons/hornetgun.h"
#include "weapons/handgrenade.h"
#include "weapons/flashbang.h"
#include "weapons/gasgrenade.h"
#include "weapons/satchel.h"
#include "weapons/timed_satchel.h"
#include "weapons/bomb.h"
#include "weapons/rpg.h"
#include "weapons/egon.h"
#include "weapons/gauss.h"
#include "weapons/m24.h"
#include "weapons/m72.h"
#include "weapons/m4.h"
#include "weapons/ak47.h"
#include <cstring>

CWeaponPredictingContext::CWeaponPredictingContext()
{
}

void CWeaponPredictingContext::PostThink(local_state_t *from, local_state_t *to, usercmd_t *cmd, bool runfuncs, double time, uint32_t randomSeed)
{
	const float previewSeenAge = gHUD.m_flTime - gHUD.m_flBuildPreviewSeenTime;
	const bool previewEntityWasSeen =
		gHUD.m_flBuildPreviewSeenTime >= 0.0f &&
		previewSeenAge >= 0.0f && previewSeenAge < 0.25f;

	if( gHUD.m_flBuildPreviewSeenTime > gHUD.m_flTime )
	{
		gHUD.m_iBuildPreviewState = 0;
		gHUD.m_flBuildPreviewSeenTime = -1.0f;
		gHUD.m_bSuppressBuildAttackUntilRelease = false;
	}

	if( previewEntityWasSeen )
		gHUD.m_iBuildPreviewState = 2;
	else if( gHUD.m_flBuildPreviewSeenTime >= 0.0f &&
		( gHUD.m_flTime - gHUD.m_flBuildPreviewSeenTime ) >= 0.25f )
		gHUD.m_iBuildPreviewState = 0;
	else if( from->client.iuser4 )
		gHUD.m_iBuildPreviewState = 2;
	else if( gHUD.m_iBuildPreviewState == 1 && time > gHUD.m_flBuildPreviewPendingUntil )
		gHUD.m_iBuildPreviewState = 0;

	if( gHUD.m_iBuildPreviewState != 0 &&
		FBitSet( cmd->buttons, IN_ATTACK | IN_ATTACK2 ))
	{
		gHUD.m_bSuppressBuildAttackUntilRelease = true;
	}

	usercmd_t predictionCmd = *cmd;
	if( gHUD.m_iBuildPreviewState != 0 ||
		gHUD.m_bSuppressBuildAttackUntilRelease )
		predictionCmd.buttons &= ~( IN_ATTACK | IN_ATTACK2 );

	if( gHUD.m_iBuildPreviewState == 0 &&
		!FBitSet( cmd->buttons, IN_ATTACK | IN_ATTACK2 ))
	{
		gHUD.m_bSuppressBuildAttackUntilRelease = false;
	}
	cmd = &predictionCmd;

	m_playerState.time = time;
	m_playerState.randomSeed = randomSeed;
	m_playerState.runfuncs = runfuncs;
	ReadPlayerState(from, to, cmd);
	ReadWeaponsState(from);

	const bool playerAlive = m_playerState.deadflag != (DEAD_DISCARDBODY + 1);
	CBaseWeaponContext *currWeapon = GetWeaponContext(from->client.m_iId);
	if (currWeapon)
	{
		if (runfuncs) {
			HandlePlayerSpawnDeath(to, currWeapon);
		}

		if (playerAlive && m_playerState.viewmodel)
		{
			if (m_playerState.nextAttack <= 0.0f) {
				currWeapon->ItemPostFrame();
			}
		}

		if (playerAlive && cmd->weaponselect)
		{
			// handle weapon switching on weapons code side
			HandleWeaponSwitch(from, to, cmd, currWeapon);
		}
		else
		{
			// does not changing weapons now, keep weapon ID same
			to->client.m_iId = from->client.m_iId;
		}

		// check for desync between local & server-side weapon animation
		if (runfuncs && (m_playerState.activeWeaponanim != m_playerState.weaponanim))
		{
			gEngfuncs.pfnWeaponAnim( m_playerState.weaponanim, 0 );
			m_playerState.activeWeaponanim = m_playerState.weaponanim;
		}
	}

	UpdatePlayerTimers(cmd);
	WritePlayerState(to);
	WriteWeaponsState(to, cmd);
}

void CWeaponPredictingContext::ReadPlayerState(const local_state_t *from, const local_state_t *to, usercmd_t *cmd)
{
	m_playerState.cached.buttons = from->playerstate.oldbuttons;
	int32_t buttonsChanged = (m_playerState.cached.buttons ^ cmd->buttons);
	m_playerState.buttonsPressed = buttonsChanged & cmd->buttons;	
	m_playerState.buttonsReleased = buttonsChanged & (~cmd->buttons);

	m_playerState.viewAngles = cmd->viewangles;
	m_playerState.buttons = cmd->buttons;
	m_playerState.flags = from->client.flags;

	m_playerState.deadflag = from->client.deadflag;
	m_playerState.waterlevel = from->client.waterlevel;
	m_playerState.maxSpeed = from->client.maxspeed;

	m_playerState.fov = from->client.fov;
	m_playerState.weaponanim = from->client.weaponanim;
	m_playerState.viewmodel = from->client.viewmodel;
	m_playerState.nextAttack = from->client.m_flNextAttack;

	// it's counterintuitive, but for some things we should take value from "to" state
	// because player movement prediction spits out calculations results in there
	// see CL_FinishPMove engine function to see list of such things.
	m_playerState.velocity = to->client.velocity;
	m_playerState.viewOffset = to->client.view_ofs;
	m_playerState.origin = to->playerstate.origin;
}

void CWeaponPredictingContext::WritePlayerState(local_state_t *to)
{
	// here we need to write back values that potentially could be modified from weapons code
	to->client.viewmodel				= m_playerState.viewmodel;
	to->client.fov						= m_playerState.fov;
	to->client.weaponanim				= m_playerState.weaponanim;
	to->client.m_flNextAttack			= m_playerState.nextAttack;
	to->client.maxspeed					= m_playerState.maxSpeed;
	to->client.velocity					= m_playerState.velocity;
}

void CWeaponPredictingContext::UpdatePlayerTimers(const usercmd_t *cmd)
{
	m_playerState.nextAttack -= cmd->msec / 1000.0;
	if (m_playerState.nextAttack < -0.001)
		m_playerState.nextAttack = -0.001;
}

void CWeaponPredictingContext::UpdateWeaponTimers(CBaseWeaponContext *weapon, const usercmd_t *cmd)
{
	weapon->m_flNextPrimaryAttack		-= cmd->msec / 1000.0;
	weapon->m_flNextSecondaryAttack		-= cmd->msec / 1000.0;
	weapon->m_flTimeWeaponIdle			-= cmd->msec / 1000.0;

	if (weapon->m_flNextPrimaryAttack < -1.0)
		weapon->m_flNextPrimaryAttack = -1.0;

	if (weapon->m_flNextSecondaryAttack < -0.001)
		weapon->m_flNextSecondaryAttack = -0.001;

	if (weapon->m_flTimeWeaponIdle < -0.001)
		weapon->m_flTimeWeaponIdle = -0.001;

	if (weapon->m_iId == WEAPON_EGON)
	{
		CEgonWeaponContext *ctx = static_cast<CEgonWeaponContext*>(weapon);
		ctx->m_flAttackCooldown -= cmd->msec / 1000.0;
		if (ctx->m_flAttackCooldown < -0.001)
			ctx->m_flAttackCooldown = -0.001;
	}
	else if (weapon->m_iId == WEAPON_GAUSS)
	{
		CGaussWeaponContext *ctx = static_cast<CGaussWeaponContext*>(weapon);
		ctx->m_flNextAmmoBurn -= cmd->msec / 1000.0;
		if (ctx->m_flNextAmmoBurn < -0.001)
			ctx->m_flNextAmmoBurn = -0.001;

		ctx->m_flAmmoStartCharge -= cmd->msec / 1000.0;
		if (ctx->m_flAmmoStartCharge < -0.001)
			ctx->m_flAmmoStartCharge = -0.001;
	}
}

void CWeaponPredictingContext::ReadWeaponsState(const local_state_t *from)
{
	for (size_t i = 0; i < MAX_LOCAL_WEAPONS; i++)
	{
		CBaseWeaponContext *weapon = GetWeaponContext(i);
		const weapon_data_t &data = from->weapondata[i];
		if (weapon)
		{
			weapon->m_fInReload				= data.m_fInReload;
			weapon->m_fInSpecialReload		= data.m_fInSpecialReload;
			weapon->m_iClip					= data.m_iClip;
			weapon->m_iReloadClipSize		= data.iuser4;
			weapon->m_flNextPrimaryAttack	= data.m_flNextPrimaryAttack;
			weapon->m_flNextSecondaryAttack	= data.m_flNextSecondaryAttack;
			weapon->m_flTimeWeaponIdle		= data.m_flTimeWeaponIdle;

			weapon->m_iSecondaryAmmoType = static_cast<int>(from->client.vuser3.z);
			weapon->m_iPrimaryAmmoType = static_cast<int>(from->client.vuser4.x);
			m_playerState.ammo[weapon->m_iPrimaryAmmoType] = static_cast<int>(from->client.vuser4.y);
			m_playerState.ammo[weapon->m_iSecondaryAmmoType] = static_cast<int>(from->client.vuser4.z);

			ReadWeaponSpecificData(weapon, from);
		}
	}
}

void CWeaponPredictingContext::WriteWeaponsState(local_state_t *to, const usercmd_t *cmd)
{
	for (size_t i = 0; i < MAX_LOCAL_WEAPONS; i++)
	{
		weapon_data_t &data = to->weapondata[i];
		CBaseWeaponContext *weapon = GetWeaponContext(i);
		if (!weapon)
		{
			std::memset(&data, 0x0, sizeof(weapon_data_t));
			continue;
		}
		else
		{
			UpdateWeaponTimers(weapon, cmd);
			data.m_fInReload				= weapon->m_fInReload;
			data.m_fInSpecialReload			= weapon->m_fInSpecialReload;
			data.m_iClip					= weapon->m_iClip; 
			data.iuser4					= weapon->m_iReloadClipSize;
			data.m_flNextPrimaryAttack		= weapon->m_flNextPrimaryAttack;
			data.m_flNextSecondaryAttack	= weapon->m_flNextSecondaryAttack;
			data.m_flTimeWeaponIdle			= weapon->m_flTimeWeaponIdle;
			WriteWeaponSpecificData(weapon, to);
		}
	}
}

// don't forget to check that according information are being sent in UpdateClientData/GetWeaponData within server/client.cpp 
void CWeaponPredictingContext::ReadWeaponSpecificData(CBaseWeaponContext *weapon, const local_state_t *from)
{
	const weapon_data_t &data = from->weapondata[weapon->m_iId];
	if (weapon->m_iId == WEAPON_RPG)
	{
		CRpgWeaponContext *ctx = static_cast<CRpgWeaponContext*>(weapon);
		ctx->m_fSpotActive = static_cast<int>(from->client.vuser2.y);
		ctx->m_cActiveRockets = static_cast<int>(from->client.vuser2.z);
	}
	else if (weapon->m_iId == WEAPON_SATCHEL)
	{
		CSatchelWeaponContext *ctx = static_cast<CSatchelWeaponContext*>(weapon);
		ctx->m_chargeReady = data.iuser1;
	}
	else if (weapon->m_iId == WEAPON_C4)
	{
		CTimedSatchelWeaponContext *ctx = static_cast<CTimedSatchelWeaponContext*>(weapon);
		ctx->m_iTimerSeconds = data.iuser2 == 30 ? 30 : 10;
	}
	else if (weapon->m_iId == WEAPON_HANDGRENADE)
	{
		CHandGrenadeWeaponContext *ctx = static_cast<CHandGrenadeWeaponContext*>(weapon);
		ctx->m_flStartThrow = data.fuser1;
		ctx->m_flReleaseThrow = data.fuser2;
		ctx->m_bWeakThrow = data.iuser1 != 0;
	}
	else if (weapon->m_iId == WEAPON_FLASHBANG)
	{
		CFlashbangWeaponContext *ctx = static_cast<CFlashbangWeaponContext*>(weapon);
		ctx->m_flStartThrow = data.fuser1;
		ctx->m_flReleaseThrow = data.fuser2;
		ctx->m_bWeakThrow = data.iuser1 != 0;
		ctx->m_bQueueNextThrow = data.iuser2 != 0;
	}
	else if (weapon->m_iId == WEAPON_GASGRENADE)
	{
		CGasGrenadeWeaponContext *ctx = static_cast<CGasGrenadeWeaponContext*>(weapon);
		ctx->m_flStartThrow = data.fuser1; ctx->m_flReleaseThrow = data.fuser2;
	}
	else if (weapon->m_iId == WEAPON_EGON)
	{
		CEgonWeaponContext *ctx = static_cast<CEgonWeaponContext*>(weapon);
		ctx->m_flAttackCooldown = data.fuser1;
	}
	else if (weapon->m_iId == WEAPON_GAUSS)
	{
		CGaussWeaponContext *ctx = static_cast<CGaussWeaponContext*>(weapon);
		ctx->m_flAmmoStartCharge = data.fuser1;
		ctx->m_flNextAmmoBurn = data.fuser2;
		ctx->m_fInAttack = data.iuser1;
	}
	else if (weapon->m_iId == WEAPON_USP)
	{
		static_cast<CUSPWeaponContext*>(weapon)->SetSilenced(data.iuser1 != 0);
	}
	else if (weapon->m_iId == WEAPON_GLOCK18)
	{
		static_cast<CGlock18WeaponContext*>(weapon)->SetFullAuto(data.iuser1 != 0);
	}
	else if (weapon->m_iId == WEAPON_M4)
	{
		static_cast<CM4WeaponContext*>(weapon)->SetSilenced(data.iuser1 != 0);
	}
}

void CWeaponPredictingContext::WriteWeaponSpecificData(CBaseWeaponContext *weapon, local_state_t *to)
{
	weapon_data_t &data = to->weapondata[weapon->m_iId];
	if (weapon->m_iId == WEAPON_RPG)
	{
		CRpgWeaponContext *ctx = static_cast<CRpgWeaponContext*>(weapon);
		to->client.vuser2.y = ctx->m_fSpotActive;
		to->client.vuser2.z = ctx->m_cActiveRockets;
	}
	else if (weapon->m_iId == WEAPON_SATCHEL)
	{
		CSatchelWeaponContext *ctx = static_cast<CSatchelWeaponContext*>(weapon);
		data.iuser1 = ctx->m_chargeReady;
	}
	else if (weapon->m_iId == WEAPON_C4)
	{
		CTimedSatchelWeaponContext *ctx = static_cast<CTimedSatchelWeaponContext*>(weapon);
		data.iuser2 = ctx->m_iTimerSeconds;
	}
	else if (weapon->m_iId == WEAPON_HANDGRENADE)
	{
		CHandGrenadeWeaponContext *ctx = static_cast<CHandGrenadeWeaponContext*>(weapon);
		data.fuser1 = ctx->m_flStartThrow;
		data.fuser2 = ctx->m_flReleaseThrow;
		data.iuser1 = ctx->m_bWeakThrow ? 1 : 0;
	}
	else if (weapon->m_iId == WEAPON_FLASHBANG)
	{
		CFlashbangWeaponContext *ctx = static_cast<CFlashbangWeaponContext*>(weapon);
		data.fuser1 = ctx->m_flStartThrow;
		data.fuser2 = ctx->m_flReleaseThrow;
		data.iuser1 = ctx->m_bWeakThrow ? 1 : 0;
		data.iuser2 = ctx->m_bQueueNextThrow ? 1 : 0;
	}
	else if (weapon->m_iId == WEAPON_GASGRENADE)
	{
		CGasGrenadeWeaponContext *ctx = static_cast<CGasGrenadeWeaponContext*>(weapon);
		data.fuser1 = ctx->m_flStartThrow; data.fuser2 = ctx->m_flReleaseThrow;
	}
	else if (weapon->m_iId == WEAPON_EGON)
	{
		CEgonWeaponContext *ctx = static_cast<CEgonWeaponContext*>(weapon);
		data.fuser1 = ctx->m_flAttackCooldown;
	}
	else if (weapon->m_iId == WEAPON_GAUSS)
	{
		CGaussWeaponContext *ctx = static_cast<CGaussWeaponContext*>(weapon);
		data.fuser1 = ctx->m_flAmmoStartCharge;
		data.fuser2 = ctx->m_flNextAmmoBurn;
		data.iuser1 = ctx->m_fInAttack;
	}
	else if (weapon->m_iId == WEAPON_USP)
	{
		data.iuser1 = static_cast<CUSPWeaponContext*>(weapon)->IsSilenced() ? 1 : 0;
	}
	else if (weapon->m_iId == WEAPON_GLOCK18)
	{
		data.iuser1 = static_cast<CGlock18WeaponContext*>(weapon)->IsFullAuto() ? 1 : 0;
	}
	else if (weapon->m_iId == WEAPON_M4)
	{
		data.iuser1 = static_cast<CM4WeaponContext*>(weapon)->IsSilenced() ? 1 : 0;
	}
}

void CWeaponPredictingContext::HandlePlayerSpawnDeath(local_state_t *to, CBaseWeaponContext *weapon)
{
	if (to->client.health <= 0 && m_playerState.cached.health > 0) {
		weapon->Holster();
	}
	else if (to->client.health > 0 && m_playerState.cached.health <= 0) {
		weapon->Deploy();
	}
	m_playerState.cached.health = to->client.health;
}

void CWeaponPredictingContext::HandleWeaponSwitch(const local_state_t *from, local_state_t *to, const usercmd_t *cmd, CBaseWeaponContext *weapon)
{
	if (from->weapondata[cmd->weaponselect].m_iId == cmd->weaponselect)
	{
		CBaseWeaponContext *selectedWeapon = GetWeaponContext(cmd->weaponselect);
		if (selectedWeapon && selectedWeapon->m_iId != weapon->m_iId)
		{
			weapon->Holster();
			selectedWeapon->Deploy();
			to->client.m_iId = cmd->weaponselect;
		}
	}
}

CBaseWeaponContext* CWeaponPredictingContext::GetWeaponContext(uint32_t weaponID)
{
	if (m_weaponsState.count(weaponID)) {
		return m_weaponsState[weaponID].get();
	}
	else
	{
		switch (weaponID)
		{	
			case WEAPON_AK47:
				m_weaponsState[weaponID] = std::make_unique<CAK47WeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_M4:
				m_weaponsState[weaponID] = std::make_unique<CM4WeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_M24:
				m_weaponsState[weaponID] = std::make_unique<CM24WeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_M72:
				m_weaponsState[weaponID] = std::make_unique<CM72WeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_BERETTA:  
				m_weaponsState[weaponID] = std::make_unique<CGlockWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_GLOCK18:
				m_weaponsState[weaponID] = std::make_unique<CGlock18WeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_CROSSBOW:
				m_weaponsState[weaponID] = std::make_unique<CCrossbowWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_PYTHON:
				m_weaponsState[weaponID] = std::make_unique<CPythonWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_MP5:
				m_weaponsState[weaponID] = std::make_unique<CMP5WeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_SHOTGUN:
				m_weaponsState[weaponID] = std::make_unique<CShotgunWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_CROWBAR:
				m_weaponsState[weaponID] = std::make_unique<CCrowbarWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_USP:
				m_weaponsState[weaponID] = std::make_unique<CUSPWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_WRENCH:
				m_weaponsState[weaponID] = std::make_unique<CWrenchWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_TRIPMINE:
				m_weaponsState[weaponID] = std::make_unique<CTripmineWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_SNARK:
				m_weaponsState[weaponID] = std::make_unique<CSqueakWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_HORNETGUN:
				m_weaponsState[weaponID] = std::make_unique<CHornetgunWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_HANDGRENADE:
				m_weaponsState[weaponID] = std::make_unique<CHandGrenadeWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_FLASHBANG:
				m_weaponsState[weaponID] = std::make_unique<CFlashbangWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_GASGRENADE:
				m_weaponsState[weaponID] = std::make_unique<CGasGrenadeWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_SATCHEL:
				m_weaponsState[weaponID] = std::make_unique<CSatchelWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_C4:
				m_weaponsState[weaponID] = std::make_unique<CTimedSatchelWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_BOMB:
				m_weaponsState[weaponID] = std::make_unique<CBombWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_RPG:
				m_weaponsState[weaponID] = std::make_unique<CRpgWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_EGON:
				m_weaponsState[weaponID] = std::make_unique<CEgonWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			case WEAPON_GAUSS:
				m_weaponsState[weaponID] = std::make_unique<CGaussWeaponContext>(std::make_unique<CClientWeaponLayerImpl>(m_playerState));
				break;
			default: 
				return nullptr;
		}

		ItemInfo itemInfo;
		m_weaponsState[weaponID]->GetItemInfo(&itemInfo);
		CBaseWeaponContext::ItemInfoArray[weaponID] = itemInfo;
		return m_weaponsState[weaponID].get();
	}
}
