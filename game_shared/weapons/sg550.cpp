#include "sg550.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 240.0f; constexpr float RELOAD_TIME = 3.25f; }

CSG550WeaponContext::CSG550WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_SG550; m_iDefaultAmmo = SG550_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/sg550.sc");
}

int CSG550WeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(SG550_CLASSNAME); info->pszAmmo1 = "556sg550";
	info->iMaxAmmo1 = SG550_MAX_CLIP * SG550_MAX_SPARE_MAGAZINES;
	info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1; info->iMaxClip = SG550_MAX_CLIP;
	info->iSlot = 0; info->iPosition = 16; info->iFlags = 0; info->iId = m_iId; info->iWeight = SG550_WEIGHT;
	return 1;
}

bool CSG550WeaponContext::Deploy()
{
	return DefaultDeploy("models/weapon/SG550/v_sg550.mdl", "models/weapon/SG550/p_sg550.mdl", SG550_DRAW, "rifle");
}

void CSG550WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0)
	{
		PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f); return;
	}
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	const float fov = m_pLayer->GetPlayerFOV();
	// Tighter than the G3SG1 while scoped; hip fire is exactly four times wider.
	float spread = IsPlayerDucking() ? 0.0f : 0.006f;
	if (fov == 0.0f) spread *= 4.0f;
	if (fov == 15.0f) spread *= 0.9f;
	if (m_pLayer->GetPlayerVelocity().Length2D() > 0.0f) spread *= 3.5f;
	if (!IsPlayerOnGround()) spread = 0.2f;
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_556,
		m_pLayer->GetRandomSeed());
	KickBack(1.25f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
	WeaponEventParams params{}; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = SG550_SHOOT1 + (m_pLayer->GetRandomSeed() & 1);
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->SetAnimation(PLAYER_ATTACK1);
	player->pev->effects |= EF_MUZZLEFLASH; player->m_iWeaponVolume = LOUD_GUN_VOLUME; player->m_iWeaponFlash = BRIGHT_GUN_FLASH;
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(FIRE_INTERVAL);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.5f;
}

void CSG550WeaponContext::SecondaryAttack()
{
	const float fov = m_pLayer->GetPlayerFOV();
	m_pLayer->SetPlayerFOV(fov == 0.0f ? 45.0f : (fov == 45.0f ? 15.0f : 0.0f));
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.3f;
}

void CSG550WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? SG550_MAX_CLIP + 1 : SG550_MAX_CLIP;
	if (m_iClip >= target) return;
	if (DefaultReload(target, SG550_RELOAD, RELOAD_TIME)) m_pLayer->SetPlayerFOV(0.0f);
}

void CSG550WeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(SG550_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 4.0f;
}

void CSG550WeaponContext::Holster()
{
	CancelReloadState(); if (m_pLayer->GetPlayerFOV() != 0.0f) m_pLayer->SetPlayerFOV(0.0f);
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
