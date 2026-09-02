/*
weapon_context.cpp - part of weapons implementation common for client & server
Copyright (C) 2024 SNMetamorph

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "weapon_context.h"
#include "weapons/famas.h"
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>

#ifdef CLIENT_DLL
#include "const.h"
#include "hud.h"
#include "utils.h"
#include "event_api.h"
#include "event_args.h"
#else
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"
#include "viewmodel_sounds.h"
#endif

ItemInfo CBaseWeaponContext::ItemInfoArray[ MAX_WEAPONS ];
AmmoInfo CBaseWeaponContext::AmmoInfoArray[ MAX_AMMO_SLOTS ];

namespace
{
using WeaponValues = std::unordered_map<std::string, float>;
std::unordered_map<std::string, WeaponValues> g_weaponConfigs;

const WeaponValues &LoadWeaponConfig(const char *weaponClassname)
{
	const std::string classname = weaponClassname ? weaponClassname : "";
	auto existing = g_weaponConfigs.find(classname);
	if (existing != g_weaponConfigs.end()) return existing->second;

	WeaponValues values;
	const std::string path = "scripts/weapons/" + classname + ".cfg";
	int length = 0;
	char *data = reinterpret_cast<char *>(LOAD_FILE(path.c_str(), &length));
	if (data)
	{
		std::string text(data, length > 0 ? static_cast<size_t>(length) : std::strlen(data));
		FREE_FILE(data);
		size_t position = 0;
		while (position < text.size())
		{
			size_t end = text.find_first_of("\r\n", position);
			std::string line = text.substr(position, end == std::string::npos ? std::string::npos : end - position);
			position = end == std::string::npos ? text.size() : end + 1;
			const size_t comment = line.find("//");
			if (comment != std::string::npos) line.erase(comment);
			const size_t hash = line.find('#');
			if (hash != std::string::npos) line.erase(hash);
			const size_t first = line.find_first_not_of(" \t{}");
			if (first == std::string::npos) continue;
			const size_t separator = line.find_first_of(" \t=", first);
			if (separator == std::string::npos) continue;
			std::string key = line.substr(first, separator - first);
			const size_t valueStart = line.find_first_not_of(" \t=\"", separator);
			if (valueStart == std::string::npos) continue;
			char *valueEnd = nullptr;
			const float value = std::strtof(line.c_str() + valueStart, &valueEnd);
			if (valueEnd != line.c_str() + valueStart) values[key] = value;
		}
	}
	return g_weaponConfigs.emplace(classname, std::move(values)).first->second;
}
}

float GetWeaponConfigValue(const char *weaponClassname, const char *key, float fallback)
{
	const WeaponValues &values = LoadWeaponConfig(weaponClassname);
	auto found = values.find(key ? key : "");
	return found == values.end() ? fallback : found->second;
}

int GetWeaponConfigInt(const char *weaponClassname, const char *key, int fallback)
{
	return static_cast<int>(GetWeaponConfigValue(weaponClassname, key, static_cast<float>(fallback)));
}

float CBaseWeaponContext::ConfigValue(const char *key, float fallback) const
{
	return GetWeaponConfigValue(const_cast<CBaseWeaponContext *>(this)->pszName(), key, fallback);
}

int CBaseWeaponContext::ConfigInt(const char *key, int fallback) const
{
	return GetWeaponConfigInt(const_cast<CBaseWeaponContext *>(this)->pszName(), key, fallback);
}

float CBaseWeaponContext::ConfigFireInterval(float fallback, bool zoomed) const
{
	const char *key = zoomed ? "zoom_rate_of_fire_rpm" : "rate_of_fire_rpm";
	const float rpm = ConfigValue(key, fallback > 0.0f ? 60.0f / fallback : 0.0f);
	return rpm > 0.0f ? 60.0f / rpm : ConfigValue("fire_interval", fallback);
}

CBaseWeaponContext::CBaseWeaponContext(std::unique_ptr<IWeaponLayer> &&layer) :
	m_pLayer(std::move(layer)),
	m_fFireOnEmpty(false),
	m_fInReload(false),
	m_fInSpecialReload(false),
	m_flNextPrimaryAttack(0.0f),
	m_flNextSecondaryAttack(0.0f),
	m_flPrevPrimaryAttack(0.0f),
	m_flLastFireTime(0.0f),
	m_flPumpTime(0.0f),
	m_flTimeWeaponIdle(0.0f),
	m_iClientClip(0),
	m_iClientWeaponState(0),
	m_iClip(0),
	m_iReloadClipSize(0),
	m_iMagazineType(0),
	m_iMagazineCapacity(0),
	m_flReloadButtonDownTime(-1.0f),
	m_bReloadTriggered(false),
	m_bTacticalReload(false),
	m_bPrimaryAttackLatched(false),
	m_iRecoilShots(0),
	m_bRecoilDirectionRight(false),
	m_flCs16PistolAccuracy(-1.0f),
	m_flCs16PistolLastFire(0.0f),
	m_iDefaultAmmo(0),
	m_iPlayEmptySound(false),
	m_iPrimaryAmmoType(0),
	m_iSecondaryAmmoType(0),
	m_iId(-1)
{
}

CBaseWeaponContext::~CBaseWeaponContext()
{
}

void CBaseWeaponContext::ItemPostFrame()
{
	if (m_iId == WEAPON_FAMAS)
		static_cast<CFamasWeaponContext*>(this)->ProcessBurstShots();
	if ((m_fInReload) && m_pLayer->GetPlayerNextAttackTime() <= m_pLayer->GetWeaponTimeBase(false))
	{
		if (UsesMagazineInventory())
		{
			m_iClip = m_pLayer->CompleteMagazineReload(m_iId, m_iPrimaryAmmoType,
				iMaxClip(), m_iClip, m_bTacticalReload);
		}
		else
		{
			const int reloadClipSize = m_iReloadClipSize ? m_iReloadClipSize : iMaxClip();
			int j = Q_min(reloadClipSize - m_iClip, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType));
			m_iClip += j;
			m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) - j);
		}

		m_fInReload = FALSE;
		m_iReloadClipSize = 0;

#ifndef CLIENT_DLL
		// Restore the equipped weapon's normal movement speed exactly when the
		// magazine reload completes; WeaponIdle may be scheduled much later.
		CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
		const char *weaponName = pszName();
		if (weaponName)
		{
			if (!strcmp(weaponName, "weapon_m4"))
				player->pev->maxspeed = 230.0f;
			else if (!strcmp(weaponName, "weapon_m24"))
				player->pev->maxspeed = 220.0f;
			else if (!strcmp(weaponName, "weapon_awp") || !strcmp(weaponName, "weapon_m72"))
				player->pev->maxspeed = 210.0f;
			else if (!strcmp(weaponName, "weapon_ak47") || !strcmp(weaponName, "weapon_galil"))
				player->pev->maxspeed = 220.0f;
			else if (!strcmp(weaponName, "weapon_m60"))
				player->pev->maxspeed = 210.0f;
			else if (!strcmp(weaponName, "weapon_m249"))
				player->pev->maxspeed = 210.0f;
			else if (!strcmp(weaponName, "weapon_beretta") || !strcmp(weaponName, "weapon_p229") || !strcmp(weaponName, "weapon_fiveseven") || !strcmp(weaponName, "weapon_usp") ||
				!strcmp(weaponName, "weapon_ragingbull") || !strcmp(weaponName, "weapon_deagle") ||
			!strcmp(weaponName, "weapon_mp5a3") || !strcmp(weaponName, "weapon_mp5sd") || !strcmp(weaponName, "weapon_mac10") || !strcmp(weaponName, "weapon_tmp") || !strcmp(weaponName, "weapon_ump") || !strcmp(weaponName, "weapon_p90") || !strcmp(weaponName, "weapon_bizon"))
				player->pev->maxspeed = 250.0f;
			player->pev->maxspeed = ConfigValue("walk_speed", player->pev->maxspeed);
		}
#endif
	}

	if (!m_pLayer->CheckPlayerButtonFlag(IN_ATTACK))
	{
		m_bPrimaryAttackLatched = false;
		m_iRecoilShots = 0;
		m_flLastFireTime = 0.0f;
		PrimaryAttackReleased();
	}

	if (m_pLayer->CheckPlayerButtonFlag(IN_ATTACK2) && CanAttack(m_flNextSecondaryAttack))
	{
		if ( pszAmmo2() && !m_pLayer->GetPlayerAmmo(SecondaryAmmoIndex()) )
		{
			m_fFireOnEmpty = TRUE;
		}

		SecondaryAttack();
		m_pLayer->ClearPlayerButtonFlag(IN_ATTACK2);
	}
	else if (m_pLayer->CheckPlayerButtonFlag(IN_ATTACK) && CanAttack(m_flNextPrimaryAttack) &&
		(!IsSemiAutomatic() || !m_bPrimaryAttackLatched))
	{
		if ( (m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && !m_pLayer->GetPlayerAmmo(PrimaryAmmoIndex())) )
		{
			m_fFireOnEmpty = TRUE;
		}

		PrimaryAttack();
		m_bPrimaryAttackLatched = true;
	}
	else if (UsesReloadTimingVariants() && iMaxClip() != WEAPON_NOCLIP && !m_fInReload &&
		(m_pLayer->CheckPlayerButtonFlag(IN_RELOAD) || m_flReloadButtonDownTime >= 0.0f))
	{
		const bool reloadDown = m_pLayer->CheckPlayerButtonFlag(IN_RELOAD);
		if (reloadDown && m_flReloadButtonDownTime < 0.0f)
		{
			m_flReloadButtonDownTime = m_pLayer->GetTime();
			m_bReloadTriggered = false;
		}
		if (reloadDown && !m_bReloadTriggered && m_pLayer->GetTime() - m_flReloadButtonDownTime >= 0.5f)
		{
			m_bTacticalReload = true;
			m_bReloadTriggered = true;
			Reload();
		}
		else if (!reloadDown && m_flReloadButtonDownTime >= 0.0f)
		{
			if (!m_bReloadTriggered)
			{
				m_bTacticalReload = false;
				Reload();
			}
			m_flReloadButtonDownTime = -1.0f;
			m_bReloadTriggered = false;
		}
	}
	else if (!UsesReloadTimingVariants() && m_pLayer->CheckPlayerButtonFlag(IN_RELOAD) && iMaxClip() != WEAPON_NOCLIP && !m_fInReload )
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ( !(m_pLayer->CheckPlayerButtonFlag(IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down

		m_fFireOnEmpty = FALSE;
		if ( IsUseable() )
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip == 0 && m_pLayer->ShouldAutoReload() &&
				!(iFlags() & ITEM_FLAG_NOAUTORELOAD) &&
				m_flNextPrimaryAttack < m_pLayer->GetWeaponTimeBase(UsePredicting()) )
			{
				Reload();
				return;
			}
		}

		WeaponIdle( );
		return;
	}
	
	// catch all
	if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}

void CBaseWeaponContext::KickBack(float upBase, float lateralBase, float upModifier,
	float lateralModifier, float upMax, float lateralMax, int directionChange)
{
	const float recoilScale = ConfigValue("recoil", 1.0f);
	upBase *= recoilScale; lateralBase *= recoilScale;
	upModifier *= recoilScale; lateralModifier *= recoilScale;
	upMax *= recoilScale; lateralMax *= recoilScale;
	++m_iRecoilShots;
	const float shot = static_cast<float>(m_iRecoilShots - 1);
	const float up = upBase + shot * upModifier;
	const float lateral = lateralBase + shot * lateralModifier;
	const Vector punch = m_pLayer->GetPlayerPunchangle();

	// Counter-Strike 1.6 style accumulation, but clamp the resulting angle as
	// well as gating it. This avoids a single high-RPM frame overshooting the
	// limit and making sustained fire physically uncomfortable.
	const float targetPitch = upMax == 0.0f ? punch.x - up : Q_max(-upMax, punch.x - up);
	float targetYaw = punch.y + (m_bRecoilDirectionRight ? lateral : -lateral);
	if (lateralMax != 0.0f)
		targetYaw = Q_max(-lateralMax, Q_min(lateralMax, targetYaw));
	m_pLayer->AddPlayerPunchangle(targetPitch - punch.x, targetYaw - punch.y, 0.0f);

	if (m_pLayer->GetRandomInt(m_pLayer->GetRandomSeed() + m_iRecoilShots,
		0, Q_max(0, directionChange)) == 0)
	{
		m_bRecoilDirectionRight = !m_bRecoilDirectionRight;
	}
}

float CBaseWeaponContext::GetCs16PistolSpread(Cs16PistolProfile profile, bool alternateMode)
{
	float maximum = 0.9f, minimum = 0.6f, recoveryWindow = 0.325f, penalty = 0.275f;
	float air = 1.0f, moving = 0.165f, ducking = 0.075f, standing = 0.1f;
	switch (profile)
	{
	case Cs16PistolProfile::USP:
		maximum = 0.92f; recoveryWindow = 0.3f; penalty = 0.275f;
		air = alternateMode ? 1.3f : 1.2f; moving = alternateMode ? 0.25f : 0.225f;
		ducking = alternateMode ? 0.125f : 0.08f; standing = alternateMode ? 0.15f : 0.1f; break;
	case Cs16PistolProfile::P228:
		maximum = 0.9f; recoveryWindow = 0.325f; penalty = 0.3f;
		air = 1.5f; moving = 0.255f; ducking = 0.075f; standing = 0.15f; break;
	case Cs16PistolProfile::Deagle:
		maximum = 0.9f; minimum = 0.55f; recoveryWindow = 0.4f; penalty = 0.35f;
		air = 1.5f; moving = 0.25f; ducking = 0.115f; standing = 0.13f; break;
	case Cs16PistolProfile::FiveSeven:
		maximum = 0.92f; minimum = 0.725f; recoveryWindow = 0.275f; penalty = 0.25f;
		air = 1.5f; moving = 0.255f; ducking = 0.075f; standing = 0.15f; break;
	case Cs16PistolProfile::Elite:
		maximum = 0.88f; minimum = 0.55f; recoveryWindow = 0.325f; penalty = 0.275f;
		air = 1.3f; moving = 0.175f; ducking = 0.08f; standing = 0.1f; break;
	case Cs16PistolProfile::Glock18:
		if (alternateMode) { air = 1.2f; moving = 0.185f; ducking = 0.095f; standing = 0.3f; }
		break;
	}

	if (m_flCs16PistolAccuracy < 0.0f) m_flCs16PistolAccuracy = maximum;
	const float accuracyForShot = m_flCs16PistolAccuracy;
	const float now = m_pLayer->GetTime();
	if (m_flCs16PistolLastFire > 0.0f)
		m_flCs16PistolAccuracy = Q_max(minimum, Q_min(maximum,
			m_flCs16PistolAccuracy - (recoveryWindow - (now - m_flCs16PistolLastFire)) * penalty));
	m_flCs16PistolLastFire = now;

	const int flags = m_pLayer->GetPlayerFlags();
	float coefficient = standing;
	if (!(flags & FL_ONGROUND)) coefficient = air;
	else if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f) coefficient = moving;
	else if (flags & FL_DUCKING) coefficient = ducking;
	return coefficient * (1.0f - accuracyForShot);
}

bool CBaseWeaponContext::IsPlayerOnGround() const
{
	return (m_pLayer->GetPlayerFlags() & FL_ONGROUND) != 0;
}

bool CBaseWeaponContext::IsPlayerDucking() const
{
	return (m_pLayer->GetPlayerFlags() & FL_DUCKING) != 0;
}

float CBaseWeaponContext::GetCs16AutomaticSpread(Cs16AutomaticProfile profile, bool silenced) const
{
	const float shots = static_cast<float>(m_iRecoilShots);
	const bool onGround = (m_pLayer->GetPlayerFlags() & FL_ONGROUND) != 0;
	const bool movingFast = m_pLayer->GetPlayerVelocity().Length2D() > 140.0f;
	float accuracy = 0.0f;

	switch (profile)
	{
	case Cs16AutomaticProfile::AK47:
		accuracy = shots == 0.0f ? 0.2f : Q_min(1.25f, shots * shots * shots / 200.0f + 0.35f);
		if (!onGround) return 0.04f + 0.4f * accuracy;
		if (movingFast) return 0.04f + 0.07f * accuracy;
		return 0.0275f * accuracy;
	case Cs16AutomaticProfile::M4A1:
		accuracy = shots == 0.0f ? 0.2f : Q_min(1.0f, shots * shots * shots / 220.0f + 0.3f);
		if (!onGround) return 0.035f + 0.4f * accuracy;
		if (movingFast) return 0.035f + 0.07f * accuracy;
		return (silenced ? 0.025f : 0.02f) * accuracy;
	case Cs16AutomaticProfile::Famas:
		accuracy = shots == 0.0f ? 0.2f : Q_min(1.0f, shots * shots * shots / 220.0f + 0.3f);
		if (!onGround) return 0.035f + 0.4f * accuracy;
		if (movingFast) return 2.0f * (0.035f + 0.07f * accuracy);
		return 0.02f * accuracy;
	case Cs16AutomaticProfile::Galil:
		accuracy = shots == 0.0f ? 0.2f : Q_min(1.125f, shots * shots * shots / 210.0f + 0.325f);
		if (!onGround) return 0.0375f + 0.4f * accuracy;
		if (movingFast) return 2.0f * 0.02375f * accuracy;
		return 0.02375f * accuracy;
	case Cs16AutomaticProfile::SG552:
		accuracy = shots == 0.0f ? 0.2f : Q_min(1.1f, shots * shots * shots / 210.0f + 0.325f);
		if (!onGround) return 0.04f + 0.42f * accuracy;
		if (movingFast) return 2.5f * 0.025f * accuracy;
		return 0.025f * accuracy;
	case Cs16AutomaticProfile::SG552Zoom:
		accuracy = shots == 0.0f ? 0.2f : Q_min(0.95f, shots * shots * shots / 230.0f + 0.275f);
		if (!onGround) return 0.03f + 0.3f * accuracy;
		if (movingFast) return 2.5f * 0.015f * accuracy;
		return 0.015f * accuracy;
	case Cs16AutomaticProfile::MP5Navy:
		accuracy = shots == 0.0f ? 0.0f : Q_min(0.75f, shots * shots / 220.1f + 0.45f);
		return (onGround ? 0.04f : 0.2f) * accuracy;
	case Cs16AutomaticProfile::M249:
		accuracy = shots == 0.0f ? 0.2f : Q_min(0.9f, shots * shots * shots / 175.0f + 0.4f);
		if (!onGround) return 0.045f + 0.5f * accuracy;
		if (movingFast) return 0.045f + 0.095f * accuracy;
		return 0.03f * accuracy;
	}
	return 0.0f;
}

void CBaseWeaponContext::ApplyCs16AutomaticKickBack(Cs16AutomaticProfile profile)
{
	const bool onGround = (m_pLayer->GetPlayerFlags() & FL_ONGROUND) != 0;
	const bool ducking = (m_pLayer->GetPlayerFlags() & FL_DUCKING) != 0;
	const bool moving = m_pLayer->GetPlayerVelocity().Length2D() > 0.0f;

	if (profile == Cs16AutomaticProfile::AK47)
	{
		if (moving) KickBack(1.5f, 0.45f, 0.225f, 0.05f, 6.5f, 2.5f, 7);
		else if (!onGround) KickBack(2.0f, 1.0f, 0.5f, 0.35f, 9.0f, 6.0f, 5);
		else if (ducking) KickBack(0.9f, 0.35f, 0.15f, 0.025f, 5.5f, 1.5f, 9);
		else KickBack(1.0f, 0.375f, 0.175f, 0.0375f, 5.75f, 1.75f, 8);
	}
	else if (profile == Cs16AutomaticProfile::M4A1)
	{
		if (moving) KickBack(1.0f, 0.45f, 0.28f, 0.045f, 3.75f, 3.0f, 7);
		else if (!onGround) KickBack(1.2f, 0.5f, 0.23f, 0.15f, 5.5f, 3.5f, 6);
		else if (ducking) KickBack(0.6f, 0.3f, 0.2f, 0.0125f, 3.25f, 2.0f, 7);
		else KickBack(0.65f, 0.35f, 0.25f, 0.015f, 3.5f, 2.25f, 7);
	}
	else if (profile == Cs16AutomaticProfile::Galil)
	{
		if (moving) KickBack(1.25f, 0.45f, 0.2525f, 0.0475f, 5.125f, 2.75f, 7);
		else if (!onGround) KickBack(1.6f, 0.75f, 0.365f, 0.25f, 7.25f, 4.75f, 6);
		else if (ducking) KickBack(0.75f, 0.325f, 0.175f, 0.01875f, 4.375f, 1.75f, 8);
		else KickBack(0.825f, 0.3625f, 0.2125f, 0.02625f, 4.625f, 2.0f, 8);
	}
	else if (profile == Cs16AutomaticProfile::SG552 || profile == Cs16AutomaticProfile::SG552Zoom)
	{
		if (moving) KickBack(1.3f, 0.47f, 0.27f, 0.05f, 5.3f, 2.9f, 7);
		else if (!onGround) KickBack(1.65f, 0.78f, 0.38f, 0.26f, 7.4f, 4.9f, 6);
		else if (ducking) KickBack(0.78f, 0.34f, 0.19f, 0.02f, 4.5f, 1.9f, 8);
		else KickBack(0.86f, 0.38f, 0.23f, 0.028f, 4.8f, 2.1f, 8);
	}
	else if (profile == Cs16AutomaticProfile::MP5Navy)
	{
		if (!onGround) KickBack(0.9f, 0.475f, 0.35f, 0.0425f, 5.0f, 3.0f, 6);
		else if (moving) KickBack(0.5f, 0.275f, 0.2f, 0.03f, 3.0f, 2.0f, 10);
		else if (ducking) KickBack(0.225f, 0.15f, 0.1f, 0.015f, 2.0f, 1.0f, 10);
		else KickBack(0.25f, 0.175f, 0.125f, 0.02f, 2.25f, 1.25f, 10);
	}
	else
	{
		if (!onGround) KickBack(1.8f, 0.65f, 0.45f, 0.125f, 5.0f, 3.5f, 8);
		else if (moving) KickBack(1.1f, 0.5f, 0.3f, 0.06f, 4.0f, 3.0f, 8);
		else if (ducking) KickBack(0.75f, 0.325f, 0.25f, 0.025f, 3.5f, 2.5f, 9);
		else KickBack(0.8f, 0.35f, 0.3f, 0.03f, 3.75f, 3.0f, 9);
	}
}

void CBaseWeaponContext::Holster()
{ 
	CancelReloadState();
	m_pLayer->DisablePlayerViewmodel();
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->weaponmodel = 0;
#endif
}

void CBaseWeaponContext::CancelReloadState()
{
	m_fInReload = FALSE; // cancel any reload in progress.
	m_iReloadClipSize = 0;
	m_flReloadButtonDownTime = -1.0f;
	m_bReloadTriggered = false;
	m_pLayer->CancelMagazineReload();
}

//=========================================================
// IsUseable - this function determines whether or not a 
// weapon is useable by the player in its current state. 
// (does it have ammo loaded? do I have any ammo for the 
// weapon?, etc)
//=========================================================
bool CBaseWeaponContext :: IsUseable()
{
	if ( m_iClip <= 0 )
	{
		if ( m_pLayer->GetPlayerAmmo( PrimaryAmmoIndex() ) <= 0 && iMaxAmmo1() != -1 )			
		{
			// clip is empty (or nonexistant) and the player has no more ammo of this type. 
			return FALSE;
		}
	}

	return TRUE;
}

bool CBaseWeaponContext :: CanDeploy()
{
	// Owned weapons can be deployed empty so they can be refilled at an Ammo Box.
	return TRUE;
}

bool CBaseWeaponContext :: DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int body )
{
	if (!CanDeploy( ))
		return FALSE;

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	const char *weaponName = pszName();
	if (weaponName)
	{
		if (!strcmp(weaponName, "weapon_m4"))
			player->pev->maxspeed = 230.0f;
		else if (!strcmp(weaponName, "weapon_ak47") || !strcmp(weaponName, "weapon_galil"))
			player->pev->maxspeed = 220.0f;
		else if (!strcmp(weaponName, "weapon_m60"))
			player->pev->maxspeed = 210.0f;
		else if (!strcmp(weaponName, "weapon_m249"))
			player->pev->maxspeed = 210.0f;
			else if (!strcmp(weaponName, "weapon_g3sg1") || !strcmp(weaponName, "weapon_sg550") || !strcmp(weaponName, "weapon_rpg") || !strcmp(weaponName, "weapon_awp") || !strcmp(weaponName, "weapon_m72"))
				player->pev->maxspeed = 210.0f;
			else if (!strcmp(weaponName, "weapon_m24"))
				player->pev->maxspeed = 220.0f;
		else if (!strcmp(weaponName, "weapon_m3") || !strcmp(weaponName, "weapon_xm1014"))
			player->pev->maxspeed = 240.0f;
		else if (!strcmp(weaponName, "weapon_crowbar") || !strcmp(weaponName, "weapon_wrench") ||
			!strcmp(weaponName, "weapon_beretta") || !strcmp(weaponName, "weapon_p229") || !strcmp(weaponName, "weapon_fiveseven") || !strcmp(weaponName, "weapon_usp") ||
			!strcmp(weaponName, "weapon_ragingbull") || !strcmp(weaponName, "weapon_357") || !strcmp(weaponName, "weapon_python") || !strcmp(weaponName, "weapon_deagle") ||
			!strcmp(weaponName, "weapon_handgrenade") || !strcmp(weaponName, "weapon_flashbang") ||
			!strcmp(weaponName, "weapon_gasgrenade") || !strcmp(weaponName, "weapon_satchel") ||
			!strcmp(weaponName, "weapon_c4") || !strcmp(weaponName, "weapon_timed_satchel") ||
			!strcmp(weaponName, "weapon_bomb") || !strcmp(weaponName, "weapon_mp5a3") || !strcmp(weaponName, "weapon_mp5sd") || !strcmp(weaponName, "weapon_mac10") || !strcmp(weaponName, "weapon_tmp") || !strcmp(weaponName, "weapon_ump") || !strcmp(weaponName, "weapon_p90") || !strcmp(weaponName, "weapon_bizon"))
			player->pev->maxspeed = 250.0f;
		player->pev->maxspeed = ConfigValue("walk_speed", player->pev->maxspeed);
	}
	player->pev->weaponmodel = MAKE_STRING(szWeaponModel);
	strcpy( player->m_szAnimExtention, szAnimExt );
#endif
	m_pLayer->SetPlayerViewmodel(szViewModel);
	SendWeaponAnim( iAnim, body );

	const float drawTime = ConfigValue("draw_time", 0.5f);
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + drawTime);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0;
	m_flLastFireTime = 0.0f;
	return TRUE;
}

BOOL CBaseWeaponContext :: DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{
	fDelay = ConfigValue("reload_time", fDelay);
	const int originalCapacity = CBaseWeaponContext::ItemInfoArray[m_iId].iMaxClip;
	if (originalCapacity > 0)
		iClipSize = iMaxClip() + Q_max(0, iClipSize - originalCapacity);
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return FALSE;

	int reloadClipSize = GetReloadClipSize(iClipSize);
	int j = Q_min(reloadClipSize - m_iClip, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType));
	if (UsesMagazineInventory())
	{
		reloadClipSize = m_pLayer->PrepareMagazineReload(m_iId, m_iPrimaryAmmoType,
			iMaxClip(), m_iClip, m_bTacticalReload);
		j = reloadClipSize >= 0 && reloadClipSize != m_iClip ? 1 : 0;
	}

	if (j == 0)
		return FALSE;

	const float reloadDelay = UsesReloadTimingVariants() && !m_bTacticalReload ?
		Q_max(0.0f, fDelay - 0.4f) : fDelay;
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + reloadDelay);

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, body );

	m_fInReload = TRUE;
	m_iReloadClipSize = reloadClipSize;

	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3;
	return TRUE;
}

int CBaseWeaponContext::GetReloadClipSize(int requestedClipSize)
{
	return Q_min(requestedClipSize, iMaxClip());
}

bool CBaseWeaponContext::UsesReloadTimingVariants()
{
	return UsesMagazineInventory();
}

void CBaseWeaponContext::SendWeaponAnim( int iAnim, int body )
{
	m_pLayer->SetPlayerWeaponAnim(iAnim);

#ifdef CLIENT_DLL
	if (m_pLayer->ShouldRunFuncs()) {
		gEngfuncs.pfnWeaponAnim(iAnim, body);
	}
#else
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	PlayViewModelSounds(m_pLayer->GetWeaponEntity(), iAnim);

	if ( UsePredicting() && ENGINE_CANSKIP( player->edict() ) )
		return;

	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, player->pev );
		WRITE_BYTE( iAnim );							// sequence number
		WRITE_BYTE( m_pLayer->GetWeaponBodygroup() );	// weaponmodel bodygroup.
	MESSAGE_END();
#endif
}

float CBaseWeaponContext::GetNextPrimaryAttackDelay(float delay)
{
	if (m_flLastFireTime <= (0.0f + FLT_EPSILON) || m_flNextPrimaryAttack <= (-1.0f + FLT_EPSILON))
 	{ 
 		// at this point, we are assuming that the client has stopped firing 
 		// and we are going to reset our book keeping variables. 
 		m_flLastFireTime = m_pLayer->GetTime(); // maybe we should use actual time instead of predicted? not obvious
 		m_flPrevPrimaryAttack = delay; 
 	} 

 	// calculate the time between this shot and the previous 
 	float flTimeBetweenFires = m_pLayer->GetTime() - m_flLastFireTime; 
 	float flCreep = 0.0f; 

	if (flTimeBetweenFires > 0.0f) {
		flCreep = flTimeBetweenFires - m_flPrevPrimaryAttack;
	}
 	 
 	m_flLastFireTime = m_pLayer->GetTime();		 
 	 
	// we need to remember what the m_flNextPrimaryAttack time is set to for each shot,
 	// store it as m_flPrevPrimaryAttack. 
 	float flNextAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + delay - flCreep; 
 	m_flPrevPrimaryAttack = flNextAttack - m_pLayer->GetWeaponTimeBase(UsePredicting()); 
 	return flNextAttack; 
}

bool CBaseWeaponContext::CanAttack(float attack_time)
{
	return (static_cast<int>(std::floor(attack_time * 1000.0)) * 1000.0f) <= m_pLayer->GetWeaponTimeBase(UsePredicting());
}

bool CBaseWeaponContext :: PlayEmptySound()
{
	if (m_iPlayEmptySound)
	{
#ifdef CLIENT_DLL
		if (m_pLayer->ShouldRunFuncs()) {
			gEngfuncs.pfnPlaySoundByNameAtLocation("weapons/357_cock1.wav", 0.8, m_pLayer->GetGunPosition());
		}
#else
		EMIT_SOUND(ENT(m_pLayer->GetWeaponEntity()->m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
#endif
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

void CBaseWeaponContext :: ResetEmptySound()
{
	m_iPlayEmptySound = 1;
}

int CBaseWeaponContext::PrimaryAmmoIndex()
{
	return m_iPrimaryAmmoType;
}

int CBaseWeaponContext::SecondaryAmmoIndex()
{
	return -1;
}

int CBaseWeaponContext::iItemSlot()				{ return 0; }	// return 0 to MAX_ITEMS_SLOTS, used in hud
int	CBaseWeaponContext::iItemPosition() 		{ return CBaseWeaponContext::ItemInfoArray[ m_iId ].iPosition; }
const char *CBaseWeaponContext::pszAmmo1() 		{ return CBaseWeaponContext::ItemInfoArray[ m_iId ].pszAmmo1; }
int CBaseWeaponContext::iMaxAmmo1()
{
	const int fallback = CBaseWeaponContext::ItemInfoArray[m_iId].iMaxAmmo1;
	const int magazines = ConfigInt("spare_magazines", -1);
	return magazines >= 0 && iMaxClip() > 0 ? iMaxClip() * magazines : fallback;
}
const char *CBaseWeaponContext::pszAmmo2() 		{ return CBaseWeaponContext::ItemInfoArray[ m_iId ].pszAmmo2; }
int	CBaseWeaponContext::iMaxAmmo2() 			{ return CBaseWeaponContext::ItemInfoArray[ m_iId ].iMaxAmmo2; }
const char *CBaseWeaponContext::pszName() 		{ return CBaseWeaponContext::ItemInfoArray[ m_iId ].pszName; }
int	CBaseWeaponContext::iMaxClip() 				{ return ConfigInt("clip_size", CBaseWeaponContext::ItemInfoArray[m_iId].iMaxClip); }
int	CBaseWeaponContext::iWeight() 				{ return ConfigInt("weight", CBaseWeaponContext::ItemInfoArray[m_iId].iWeight); }
int CBaseWeaponContext::iFlags() 				{ return CBaseWeaponContext::ItemInfoArray[ m_iId ].iFlags; }
