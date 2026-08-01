#include "famas.h"
#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include "user_messages.h"
#endif

namespace { constexpr float FIRE_INTERVAL = 60.0f / 1000.0f; constexpr float RELOAD_TIME = 3.3f; constexpr float BURST_COOLDOWN = 0.20f; }

CFamasWeaponContext::CFamasWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_FAMAS;
	m_iDefaultAmmo = FAMAS_DEFAULT_GIVE;
	m_usFireEvent = m_pLayer->PrecacheEvent("events/famas.sc");
}

int CFamasWeaponContext::GetItemInfo(ItemInfo* info) const
{
	info->pszName = CLASSNAME_STR(FAMAS_CLASSNAME); info->pszAmmo1 = "556";
	info->iMaxAmmo1 = FAMAS_MAX_CLIP * FAMAS_MAX_SPARE_MAGAZINES; info->pszAmmo2 = nullptr; info->iMaxAmmo2 = -1;
	info->iMaxClip = FAMAS_MAX_CLIP; info->iSlot = 0; info->iPosition = 13;
	info->iFlags = 0; info->iId = m_iId; info->iWeight = FAMAS_WEIGHT; return 1;
}

int CFamasWeaponContext::GetReloadClipSize(int requestedClipSize) { return requestedClipSize; }
bool CFamasWeaponContext::Deploy() { return DefaultDeploy("models/weapon/Famas/v_famas.mdl", "models/weapon/Famas/p_famas.mdl", FAMAS_DRAW, "famas"); }

void CFamasWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerWaterlevel() == 3 || m_iClip <= 0) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.15f); return; }
	if (m_bBurstMode)
		m_iBurstShotsRemaining = Q_min(3, m_iClip);
	FireShot(m_bBurstMode ? 1 : 0);
}

void CFamasWeaponContext::FireShot(int burstBullet)
{
	--m_iClip;
	Vector source = m_pLayer->GetGunPosition(); matrix3x3 camera = m_pLayer->GetCameraOrientation();
	camera.SetForward(m_pLayer->GetAutoaimVector(AUTOAIM_5DEGREES));
	int damage = 30;
#ifndef CLIENT_DLL
	damage = static_cast<int>(gSkillData.plrDmgFamas);
#endif
	const int savedRecoilShots = m_iRecoilShots;
	if (burstBullet > 0)
		m_iRecoilShots = burstBullet <= 2 ? 0 : 1;
	const float spread = GetCs16AutomaticSpread(Cs16AutomaticProfile::Famas);
	m_iRecoilShots = savedRecoilShots;
	Vector direction = m_pLayer->FireBullets(1, source, camera, 8192.0f, spread, BULLET_PLAYER_556, m_pLayer->GetRandomSeed(), damage);
	ApplyCs16AutomaticKickBack(Cs16AutomaticProfile::M4A1);
	WeaponEventParams params; params.flags = WeaponEventFlags::NotHost; params.eventindex = m_usFireEvent; params.delay = 0.0f;
	params.origin = source; params.angles = camera.GetAngles(); params.fparam1 = direction.x; params.fparam2 = direction.y;
	params.iparam1 = params.iparam2 = 0; params.bparam1 = params.bparam2 = 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(params);
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	player->m_iWeaponFlash = NORMAL_GUN_FLASH; player->pev->effects |= EF_MUZZLEFLASH; player->SetAnimation(PLAYER_ATTACK1);
#endif
	if (burstBullet > 0)
	{
		--m_iBurstShotsRemaining;
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(m_iBurstShotsRemaining > 0 ? FIRE_INTERVAL : BURST_COOLDOWN);
	}
	else
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(FIRE_INTERVAL);
	m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.0f;
}

void CFamasWeaponContext::ProcessBurstShots()
{
	if (m_iBurstShotsRemaining > 0 && CanAttack(m_flNextPrimaryAttack))
		FireShot(4 - m_iBurstShotsRemaining);
}

void CFamasWeaponContext::SecondaryAttack()
{
	m_bBurstMode = !m_bBurstMode;
	const float now = m_pLayer->GetWeaponTimeBase(UsePredicting());
	m_flNextSecondaryAttack = now + 0.3f;
#ifndef CLIENT_DLL
	CBasePlayer* player = m_pLayer->GetWeaponEntity()->m_pPlayer;
	MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, NULL, player->pev);
		WRITE_BYTE(HUD_PRINTCENTER);
		WRITE_STRING(m_bBurstMode ? "Fire mode: Burst" : "Fire mode: Full auto");
		WRITE_STRING(""); WRITE_STRING(""); WRITE_STRING(""); WRITE_STRING("");
	MESSAGE_END();
#endif
}

void CFamasWeaponContext::Reload()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0) return;
	const int target = m_iClip > 0 ? FAMAS_MAX_CLIP + 1 : FAMAS_MAX_CLIP;
	if (m_iClip >= target) return;
	DefaultReload(target, FAMAS_RELOAD, RELOAD_TIME);
}

void CFamasWeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	SendWeaponAnim(FAMAS_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}

void CFamasWeaponContext::Holster()
{
	m_iBurstShotsRemaining = 0; CancelReloadState(); m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
}
