#include "g3sg1.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 240.0f; constexpr float RELOAD_TIME = 3.25f; }

CG3SG1WeaponContext::CG3SG1WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_G3SG1; m_iDefaultAmmo = G3SG1_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/g3sg1.sc");
}

int CG3SG1WeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(G3SG1_CLASSNAME); info->pszAmmo1 = "762g3sg1";
	info->iMaxAmmo1 = G3SG1_MAX_CLIP * G3SG1_MAX_SPARE_MAGAZINES;
	info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1; info->iMaxClip = G3SG1_MAX_CLIP;
	info->iSlot = 0; info->iPosition = 15; info->iFlags = 0; info->iId = m_iId; info->iWeight = G3SG1_WEIGHT;
	return 1;
}

bool CG3SG1WeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/G3SG1/v_g3sg1.mdl", "models/weapon/G3SG1/p_g3sg1.mdl", G3SG1_DRAW, "rifle");
}

void CG3SG1WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f); return;
	}
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	const float fov = m_pLayer->GetPlayerFOV();
	const float m24AimedSpread = IsPlayerDucking() ? 0.0f : 0.007f;
	float spread = fov == 0.0f ? m24AimedSpread * 4.0f : m24AimedSpread;
	// The 6x scope tightens the aimed group by a further ten percent.
	if (fov == 15.0f) spread *= 0.9f;
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f) spread *= 3.5f;
	if (!IsPlayerOnGround()) spread = 0.2f;
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_762,
		m_pLayer->GetRandomSeed());
	KickBack(1.5f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
	WeaponEventParams params{}; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = G3SG1_SHOOT1 + (m_pLayer->GetRandomSeed() & 1);
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->SetAnimation(PLAYER_ATTACK1);
	player->pev->effects |= EF_MUZZLEFLASH; player->m_iWeaponVolume = LOUD_GUN_VOLUME; player->m_iWeaponFlash = BRIGHT_GUN_FLASH;
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(FIRE_INTERVAL);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.5f;
}

void CG3SG1WeaponContext::SecondaryAttack()
{
	const float fov = m_pLayer->GetPlayerFOV();
	m_pLayer->SetPlayerFOV(fov == 0.0f ? 45.0f : (fov == 45.0f ? 15.0f : 0.0f));
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.3f;
}

void CG3SG1WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? G3SG1_MAX_CLIP + 1 : G3SG1_MAX_CLIP;
	if (m_iClip >= target) return;
	if (DefaultReload(target, G3SG1_RELOAD, RELOAD_TIME)) m_pLayer->SetPlayerFOV(0.0f);
}

void CG3SG1WeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(G3SG1_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 4.0f;
}

void CG3SG1WeaponContext::Holster()
{
	CancelReloadState(); if (m_pLayer->GetPlayerFOV() != 0.0f) m_pLayer->SetPlayerFOV(0.0f);
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
