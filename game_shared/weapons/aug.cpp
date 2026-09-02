#include "aug.h"
#include "const.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 730.0f; constexpr float ZOOM_FIRE_INTERVAL = 60.0f / 555.0f; constexpr float RELOAD_TIME = 3.0f; }

CAUGWeaponContext::CAUGWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_AUG; m_iDefaultAmmo = AUG_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/aug.sc");
}

int CAUGWeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(AUG_CLASSNAME); info->pszAmmo1 = "556";
	info->iMaxAmmo1 = _556_MAX_CARRY; info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1;
	info->iMaxClip = AUG_MAX_CLIP; info->iSlot = 0; info->iPosition = 15;
	info->iFlags = 0; info->iId = m_iId; info->iWeight = AUG_WEIGHT; return 1;
}

int CAUGWeaponContext::GetReloadClipSize(int requestedClipSize) { return requestedClipSize; }
bool CAUGWeaponContext::Deploy() { return DefaultDeploy("models/weapon/AUG/v_aug.mdl", "models/weapon/AUG/p_aug.mdl", AUG_DRAW, "aug"); }

void CAUGWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f); return; }
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	int damage = 28;
#ifndef CLIENT_DLL
	damage = static_cast<int>(gSkillData.plrDmgAUG);
#endif
	const bool zoomed = m_pLayer->GetPlayerFOV() != 0.0f;
	float spread = zoomed ? GetCs16AutomaticSpread(Cs16AutomaticProfile::SG552Zoom) * 0.9f
		: GetCs16AutomaticSpread(Cs16AutomaticProfile::M4A1);
	if (!zoomed && m_pLayer->GetPlayerVelocity().Length2D() > 140.0f && (m_pLayer->GetPlayerFlags() & FL_ONGROUND)) spread *= 2.5f;
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_556, m_pLayer->GetRandomSeed(), damage);
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::M4A1);
	WeaponEventParams params; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent; params.delay = 0.0f;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = params.iparam2 = 0; params.bparam1 = params.bparam2 = 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH; player->pev->effects |= EF_MUZZLEFLASH; player->SetAnimation(PLAYER_ATTACK1);
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(ConfigFireInterval(zoomed ? ZOOM_FIRE_INTERVAL : FIRE_INTERVAL, zoomed)); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + ConfigValue("idle_time", 1.0f);
}

void CAUGWeaponContext::SecondaryAttack()
{
	// GoldSrc uses a 90-degree default FOV, so 30 degrees is a true 3x zoom.
	m_pLayer->SetPlayerFOV(m_pLayer->GetPlayerFOV() != 0.0f ? 0.0f : 30.0f);
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + ConfigValue("mode_switch_time", 0.3f);
}

void CAUGWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? AUG_MAX_CLIP + 1 : AUG_MAX_CLIP; if (m_iClip >= target) return;
	DefaultReload(target, AUG_RELOAD, ConfigValue("reload_time", RELOAD_TIME));
}

void CAUGWeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(AUG_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.0f;
}

void CAUGWeaponContext::Holster()
{
	CancelReloadState();
	if (m_pLayer->GetPlayerFOV() != 0.0f) m_pLayer->SetPlayerFOV(0.0f);
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
