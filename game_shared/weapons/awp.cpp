#include "awp.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

namespace
{
	constexpr float FIRE_ANIMATION_TIME = 75.0f / 40.0f;
	constexpr float RELOAD_TIME = 2.5f;
}

CAWPWeaponContext::CAWPWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_AWP;
	m_iDefaultAmmo = AWP_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/awp.sc");
}

int CAWPWeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(AWP_CLASSNAME); info->pszAmmo1 = "338awp";
	info->iMaxAmmo1 = AWP_MAX_CLIP * AWP_MAX_SPARE_MAGAZINES;
	info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1; info->iMaxClip = AWP_MAX_CLIP;
	info->iSlot = 0; info->iPosition = 17; info->iFlags = 0; info->iId = m_iId; info->iWeight = AWP_WEIGHT;
	return 1;
}

bool CAWPWeaponContext::Deploy()
{
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 210.0f;
#endif
	return DefaultDeploy("models/weapon/AWP/v_awp.mdl", "models/p_9mmAR.mdl", AWP_DRAW, "rifle");
}

void CAWPWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f); return;
	}
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	const float fov = m_pLayer->GetPlayerFOV();
	float spread = 0.001f; // extremely accurate, but deliberately non-zero
	if (fov == 0.0f) spread *= 6.0f;
	if (fov == 11.25f) spread *= 0.9f; // second (8x) zoom is 10% more accurate
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f) spread *= 6.5f;
	if (!IsPlayerOnGround()) spread *= 2.0f;
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_762,
		m_pLayer->GetRandomSeed());
	KickBack(2.0f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
	WeaponEventParams params{}; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = AWP_FIRE1 + (m_pLayer->GetRandomSeed() % 3);
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
	if (fov != 0.0f)
	{
		m_flRestoreZoomFov = fov;
		m_pLayer->SetPlayerFOV(0.0f);
	}
#ifndef CLIENT_DLL
	// The fire event starts the viewmodel animation on clients, but model sound
	// events are scheduled on the server so bolt sounds are audible to everyone.
	SendWeaponAnim(params.iparam1);
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->SetAnimation(PLAYER_ATTACK1);
	player->pev->effects |= EF_MUZZLEFLASH; player->m_iWeaponVolume = LOUD_GUN_VOLUME; player->m_iWeaponFlash = BRIGHT_GUN_FLASH;
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(FIRE_ANIMATION_TIME);
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + FIRE_ANIMATION_TIME;
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + FIRE_ANIMATION_TIME;
}

void CAWPWeaponContext::SecondaryAttack()
{
	const float fov = m_pLayer->GetPlayerFOV();
	m_pLayer->SetPlayerFOV(fov == 0.0f ? 30.0f : (fov == 30.0f ? 11.25f : 0.0f));
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.3f;
}

void CAWPWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? AWP_MAX_CLIP + 1 : AWP_MAX_CLIP;
	if (m_iClip >= target) return;
	if (DefaultReload(target, AWP_RELOAD, RELOAD_TIME)) m_pLayer->SetPlayerFOV(0.0f);
}

void CAWPWeaponContext::WeaponIdle()
{
	ResetEmptySound();
	const float now = m_pLayer->GetWeaponTimeBase(UsePredicting());
	if (m_flTimeWeaponIdle > now) return;
	if (m_flRestoreZoomFov != 0.0f)
	{
		if (m_iClip > 0)
			m_pLayer->SetPlayerFOV(m_flRestoreZoomFov);
		m_flRestoreZoomFov = 0.0f;
	}
	SendWeaponAnim(AWP_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 4.0f;
}

void CAWPWeaponContext::Holster()
{
	CancelReloadState(); if (m_pLayer->GetPlayerFOV() != 0.0f) m_pLayer->SetPlayerFOV(0.0f);
	m_flRestoreZoomFov = 0.0f;
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 0.0f;
#endif
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
