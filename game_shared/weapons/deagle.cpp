#include "deagle.h"
#include <utility>

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#endif

namespace
{
constexpr float DEAGLE_STANDING_SPREAD = 0.01f;
constexpr float DEAGLE_MOVING_SPREAD = DEAGLE_STANDING_SPREAD * 5.0f;
constexpr float DEAGLE_FIRE_INTERVAL = 60.0f / 260.0f;
}

CDeagleWeaponContext::CDeagleWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_DEAGLE;
	m_iDefaultAmmo = DEAGLE_DEFAULT_GIVE;
	m_usFireDeagle = m_pLayer->PrecacheEvent("events/deagle.sc");
}

int CDeagleWeaponContext::GetItemInfo(ItemInfo *info) const
{
	info->pszName = CLASSNAME_STR(DEAGLE_CLASSNAME);
	info->pszAmmo1 = "50ae";
	info->iMaxAmmo1 = DEAGLE_MAX_CLIP * DEAGLE_MAX_SPARE_MAGAZINES;
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = DEAGLE_MAX_CLIP;
	info->iSlot = 1;
	info->iPosition = 5;
	info->iFlags = 0;
	info->iId = m_iId;
	info->iWeight = DEAGLE_WEIGHT;
	return 1;
}

bool CDeagleWeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/DEagle/v_deagle.mdl", "models/weapon/DEagle/p_deagle.mdl", DEAGLE_DRAW, "onehanded");
}

void CDeagleWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f);
		return;
	}

	--m_iClip;
	Vector source = m_pLayer->GetGunPosition();
	matrix3x3 aim = m_pLayer->GetCameraOrientation();
	aim.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_10DEGREES));
	const float spread = m_pLayer->GetPlayerVelocity().Length2D() > 0.0f ? DEAGLE_MOVING_SPREAD : DEAGLE_STANDING_SPREAD;
	Vector direction = m_pLayer->FireBullets(1, source, aim, 8192, spread, BULLET_PLAYER_50AE, m_pLayer->GetRandomSeed());

	WeaponEventParams params{};
	params.flags = WeaponEventFlags::NotHost;
	params.eventindex = m_usFireDeagle;
	params.origin = source;
	params.angles = aim.GetAngles();
	params.fparam1 = direction.x;
	params.fparam2 = direction.y;
	params.iparam1 = m_iClip ? DEAGLE_SHOOT1 + (m_pLayer->GetRandomSeed() % 2) : DEAGLE_SHOOT_EMPTY;
	params.bparam1 = m_iClip == 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);

#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	player->SetAnimation(PLAYER_ATTACK1);
	player->pev->effects |= EF_MUZZLEFLASH;
	player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH;
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(DEAGLE_FIRE_INTERVAL);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 2.0f;
}

void CDeagleWeaponContext::Reload()
{
	const int reloadSize = m_iClip > 0 ? DEAGLE_MAX_CLIP + 1 : DEAGLE_MAX_CLIP;
	DefaultReload(reloadSize, DEAGLE_RELOAD, 2.2f);
}

void CDeagleWeaponContext::WeaponIdle()
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	// The empty-shot sequence leaves the slide locked open. Do not play the
	// regular idle sequence until a magazine has been loaded, because that
	// sequence resets the slide to its forward position.
	if (m_iClip <= 0)
	{
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f;
		return;
	}
	SendWeaponAnim(DEAGLE_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 4.0f;
}
