#include "m249.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 750.0f; constexpr float RELOAD_TIME = 216.0f / 48.0f; constexpr float DRAW_TIME = 45.0f / 39.0f; }

CM249WeaponContext::CM249WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_M249; m_iDefaultAmmo = M249_DEFAULT_GIVE; m_usFireEvent = m_pLayer->PrecacheEvent("events/m249.sc");
}
int CM249WeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(M249_CLASSNAME); info->pszAmmo1 = "556belt"; info->iMaxAmmo1 = M249_MAX_CLIP * M249_MAX_SPARE_MAGAZINES;
	info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1; info->iMaxClip = M249_MAX_CLIP; info->iSlot = 0; info->iPosition = 11;
	info->iFlags = 0; info->iId = m_iId; info->iWeight = M249_WEIGHT; return 1;
}
int CM249WeaponContext::GetReloadClipSize(int requestedClipSize) { return requestedClipSize; }
bool CM249WeaponContext::Deploy()
{
	m_iShotsFired = 0; const bool ok = DefaultDeploy("models/weapon/M249/v_m249_mirror.mdl", "models/weapon/M249/p_m249.mdl", M249_DRAW, "m249");
	if (ok) { const float now = m_pLayer->GetWeaponTimeBase(UsePredicting()); m_pLayer->SetPlayerNextAttackTime(now + DRAW_TIME); m_flNextPrimaryAttack = now + DRAW_TIME; }
	return ok;
}
void CM249WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f); return; }
	--m_iClip; Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation(); camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	int damage = 35;
#ifndef CLIENT_DLL
	damage = static_cast<int>(gSkillData.plrDmgM249);
#endif
	float spread = GetCs16AutomaticSpread(Cs16AutomaticProfile::AK47);
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f) spread *= 2.5f;
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_556, m_pLayer->GetRandomSeed(), damage);
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::AK47); ++m_iShotsFired;
	WeaponEventParams params; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent; params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y; params.iparam1 = m_iShotsFired & 1;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->m_iWeaponVolume = LOUD_GUN_VOLUME; player->m_iWeaponFlash = BRIGHT_GUN_FLASH; player->pev->effects |= EF_MUZZLEFLASH; player->SetAnimation(PLAYER_ATTACK1);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(ConfigFireInterval(FIRE_INTERVAL)); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + ConfigValue("idle_time", 1.0f);
}
void CM249WeaponContext::Reload() { m_iShotsFired = 0; if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0 && m_iClip < M249_MAX_CLIP) DefaultReload(M249_MAX_CLIP, M249_RELOAD, ConfigValue("reload_time", RELOAD_TIME)); }
void CM249WeaponContext::WeaponIdle() { ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return; SendWeaponAnim(M249_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f; }
void CM249WeaponContext::Holster() { m_iShotsFired = 0; CancelReloadState(); m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f); }
