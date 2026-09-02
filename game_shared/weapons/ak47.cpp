#include "ak47.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include "weapon_ak47.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 550.0f; constexpr float RELOAD_TIME = 2.45f; }

CAK47WeaponContext::CAK47WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_AK47; m_iDefaultAmmo = AK47_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/ak47.sc");
}

int CAK47WeaponContext::GetItemInfo(ItemInfo *info) const
{
	info->pszName = CLASSNAME_STR(AK47_CLASSNAME); info->pszAmmo1 = "762x39";
	info->iMaxAmmo1 = _762X39_MAX_CARRY; info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1;
	info->iMaxClip = AK47_MAX_CLIP; info->iSlot = 0; info->iPosition = 9;
	info->iFlags = 0; info->iId = m_iId; info->iWeight = AK47_WEIGHT; return 1;
}

int CAK47WeaponContext::GetReloadClipSize(int requestedClipSize) { return requestedClipSize; }
bool CAK47WeaponContext::Deploy()
{
	const bool deployed = DefaultDeploy("models/weapon/AK-47/v_ak47.mdl", "models/p_9mmAR.mdl", AK47_DRAW, "ak47");
	return deployed;
}

void CAK47WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f); return; }
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	int damage = 36;
#ifndef CLIENT_DLL
	damage = static_cast<int>(gSkillData.plrDmgAK47);
#endif
	const float spread = GetCs16AutomaticSpread(Cs16AutomaticProfile::AK47);
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_762X39, m_pLayer->GetRandomSeed(), damage);
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::AK47);
	WeaponEventParams params; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent; params.delay = 0.0f;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = params.iparam2 = 0; params.bparam1 = params.bparam2 = 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH; player->pev->effects |= EF_MUZZLEFLASH; player->SetAnimation(PLAYER_ATTACK1);
	if (!m_iClip && player->m_rgAmmo[m_iPrimaryAmmoType] <= 0) player->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(ConfigFireInterval(FIRE_INTERVAL)); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + ConfigValue("idle_time", 1.9f);
}

void CAK47WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? AK47_MAX_CLIP + 1 : AK47_MAX_CLIP; if (m_iClip >= target) return;
	if (DefaultReload(target, AK47_RELOAD, ConfigValue("reload_time", RELOAD_TIME))) {
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = ConfigValue("reload_walk_speed", 160.0f);
#endif
	}
}

void CAK47WeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = ConfigValue("walk_speed", 220.0f);
#endif
	SendWeaponAnim(AK47_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}

void CAK47WeaponContext::Holster()
{
	CancelReloadState();
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 0.0f;
#endif
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
