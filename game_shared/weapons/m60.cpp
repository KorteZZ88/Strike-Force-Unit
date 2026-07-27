#include "m60.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#endif

namespace
{
constexpr float FIRE_INTERVAL = 60.0f / 550.0f;
constexpr float RELOAD_TIME = 5.8f; // 174 frames at 30 fps in weapon/M60.
constexpr float DRAW_TIME = 1.0f;   // 31 frames at 30 fps in weapon/M60.
}

CM60WeaponContext::CM60WeaponContext(std::unique_ptr<IWeaponLayer> &&layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_M60;
	m_iDefaultAmmo = M60_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/m60.sc");
}

int CM60WeaponContext::GetItemInfo(ItemInfo *info) const
{
	info->pszName = CLASSNAME_STR(M60_CLASSNAME);
	info->pszAmmo1 = "762belt";
	info->iMaxAmmo1 = M60_MAX_CLIP * M60_MAX_SPARE_MAGAZINES;
	info->pszAmmo2 = nullptr;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = M60_MAX_CLIP;
	info->iSlot = 0;
	info->iPosition = 10;
	info->iFlags = 0;
	info->iId = m_iId;
	info->iWeight = M60_WEIGHT;
	return 1;
}

int CM60WeaponContext::GetReloadClipSize(int requestedClipSize) { return requestedClipSize; }

bool CM60WeaponContext::Deploy()
{
	m_iShotsFired = 0;
	const bool deployed = DefaultDeploy("models/weapon/M60/v_m60.mdl", "models/weapon/M60/p_m60.mdl", M60_DRAW, "m60");
	if (deployed)
	{
		m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + DRAW_TIME);
		m_flNextPrimaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + DRAW_TIME;
	}
	return deployed;
}

void CM60WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f);
		return;
	}

	--m_iClip;
	Vector source = m_pLayer->GetGunPosition();
	matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	int damage = 39;
#ifndef CLIENT_DLL
	damage = static_cast<int>(gSkillData.plrDmgM60);
#endif
	// The opening shot is deliberately tight; sustained fire is less accurate than the AK-47.
	const float spread = m_iShotsFired == 0 ? 0.012f : Q_min(0.060f, 0.032f + m_iShotsFired * 0.002f);
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_762, m_pLayer->GetRandomSeed(), damage);
	++m_iShotsFired;

	WeaponEventParams params;
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usFireEvent;
	params.origin = source;
	params.angles = camera.GetAngles();
	params.fparam1 = direction.x;
	params.fparam2 = direction.y;
	params.iparam1 = m_iShotsFired & 1;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	player->pev->effects |= EF_MUZZLEFLASH;
	player->SetAnimation(PLAYER_ATTACK1);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(FIRE_INTERVAL);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f;
}

void CM60WeaponContext::Reload()
{
	m_iShotsFired = 0;
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0 || m_iClip >= M60_MAX_CLIP) return;
	DefaultReload(M60_MAX_CLIP, M60_RELOAD, RELOAD_TIME);
}

void CM60WeaponContext::WeaponIdle()
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(M60_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}

void CM60WeaponContext::Holster()
{
	m_iShotsFired = 0;
	CancelReloadState();
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
