#include "galil.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include "weapon_galil.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 666.0f; constexpr float RELOAD_TIME = 3.2f; }

CGalilWeaponContext::CGalilWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_GALIL; m_iDefaultAmmo = GALIL_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/galil.sc");
}

int CGalilWeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(GALIL_CLASSNAME); info->pszAmmo1 = "556";
	info->iMaxAmmo1 = _556_MAX_CARRY; info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1;
	info->iMaxClip = GALIL_MAX_CLIP; info->iSlot = 0; info->iPosition = 12;
	info->iFlags = 0; info->iId = m_iId; info->iWeight = GALIL_WEIGHT; return 1;
}

int CGalilWeaponContext::GetReloadClipSize(int requestedClipSize) { return requestedClipSize; }
bool CGalilWeaponContext::Deploy() { return DefaultDeploy("models/weapon/Galil/v_galil.mdl", "models/weapon/Galil/p_galil.mdl", GALIL_DRAW, "galil"); }

void CGalilWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f); return; }
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	int damage = 30;
#ifndef CLIENT_DLL
	damage = static_cast<int>(gSkillData.plrDmgGalil);
#endif
	const float spread = GetCs16AutomaticSpread(Cs16AutomaticProfile::Galil);
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_556, m_pLayer->GetRandomSeed(), damage);
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::Galil);
	WeaponEventParams params; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent; params.delay = 0.0f;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = params.iparam2 = 0; params.bparam1 = params.bparam2 = 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH; player->pev->effects |= EF_MUZZLEFLASH; player->SetAnimation(PLAYER_ATTACK1);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(FIRE_INTERVAL); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f;
}

void CGalilWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? GALIL_MAX_CLIP + 1 : GALIL_MAX_CLIP; if (m_iClip >= target) return;
	DefaultReload(target, GALIL_RELOAD, RELOAD_TIME);
}

void CGalilWeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(GALIL_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.35f;
}

void CGalilWeaponContext::Holster()
{
	CancelReloadState(); m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
