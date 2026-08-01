#include "sg552.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 700.0f; constexpr float ZOOM_FIRE_INTERVAL = 60.0f / 555.0f; constexpr float RELOAD_TIME = 2.8f; }

CSG552WeaponContext::CSG552WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_SG552; m_iDefaultAmmo = SG552_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/sg552.sc");
}

int CSG552WeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(SG552_CLASSNAME); info->pszAmmo1 = "556";
	info->iMaxAmmo1 = _556_MAX_CARRY; info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1;
	info->iMaxClip = SG552_MAX_CLIP; info->iSlot = 0; info->iPosition = 14;
	info->iFlags = 0; info->iId = m_iId; info->iWeight = SG552_WEIGHT; return 1;
}

int CSG552WeaponContext::GetReloadClipSize(int requestedClipSize) { return requestedClipSize; }
bool CSG552WeaponContext::Deploy() { return DefaultDeploy("models/weapon/SG552/v_sg552.mdl", "models/weapon/SG552/p_sg552.mdl", SG552_DRAW, "sg552"); }

void CSG552WeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f); return; }
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	int damage = 30;
#ifndef CLIENT_DLL
	damage = static_cast<int>(gSkillData.plrDmgSG552);
#endif
	const bool zoomed = m_pLayer->GetPlayerFOV() != 0.0f;
	const float spread = GetCs16AutomaticSpread(zoomed ? Cs16AutomaticProfile::SG552Zoom : Cs16AutomaticProfile::SG552);
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_556, m_pLayer->GetRandomSeed(), damage);
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::SG552);
	WeaponEventParams params; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent; params.delay = 0.0f;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = params.iparam2 = 0; params.bparam1 = params.bparam2 = 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH; player->pev->effects |= EF_MUZZLEFLASH; player->SetAnimation(PLAYER_ATTACK1);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(zoomed ? ZOOM_FIRE_INTERVAL : FIRE_INTERVAL); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f;
}

void CSG552WeaponContext::SecondaryAttack()
{
	// Default GoldSrc FOV is 90 degrees; 30 degrees gives a true 3x zoom.
	m_pLayer->SetPlayerFOV(m_pLayer->GetPlayerFOV() != 0.0f ? 0.0f : 30.0f);
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.3f;
}

void CSG552WeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? SG552_MAX_CLIP + 1 : SG552_MAX_CLIP; if (m_iClip >= target) return;
	DefaultReload(target, SG552_RELOAD, RELOAD_TIME);
}

void CSG552WeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(SG552_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.0f;
}

void CSG552WeaponContext::Holster()
{
	CancelReloadState();
	if (m_pLayer->GetPlayerFOV() != 0.0f) m_pLayer->SetPlayerFOV(0.0f);
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
