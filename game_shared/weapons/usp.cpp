#include "usp.h"
#include <utility>

#ifdef CLIENT_DLL
#else
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#endif

CUSPWeaponContext::CUSPWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_USP;
	m_iDefaultAmmo = USP_DEFAULT_GIVE;
	m_usFireUSP = m_pLayer->PrecacheEvent("events/usp.sc");
}

int CUSPWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(USP_CLASSNAME); p->pszAmmo1 = "45acp"; p->iMaxAmmo1 = _45ACP_MAX_CARRY;
	p->pszAmmo2 = NULL; p->iMaxAmmo2 = -1; p->iMaxClip = USP_MAX_CLIP; p->iSlot = 1; p->iPosition = 2;
	p->iFlags = 0; p->iId = m_iId; p->iWeight = USP_WEIGHT; return 1;
}

bool CUSPWeaponContext::Deploy()
{
	m_flAccuracy = 0.92f;
	m_pLayer->SetWeaponBodygroup(0);
	return DefaultDeploy("models/weapon/USP/v_usp.mdl", "models/weapon/USP/p_usp.mdl", m_bSilenced ? USP_DRAW : USP_UNSIL_DRAW, "onehanded", 0);
}

void CUSPWeaponContext::SecondaryAttack()
{
	m_bSilenced = !m_bSilenced;
	m_pLayer->SetWeaponBodygroup(0);
	SendWeaponAnim(m_bSilenced ? USP_ATTACH_SILENCER : USP_DETACH_SILENCER, 0);
	const float now = m_pLayer->GetWeaponTimeBase(UsePredicting());
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(3.0f);
	m_flNextSecondaryAttack = now + 3.0f;
	m_flTimeWeaponIdle = now + 3.0f;
}

void CUSPWeaponContext::PrimaryAttack()
{
	const bool airborne = false;
	const bool moving = m_pLayer->GetPlayerVelocity().Length2D() > 0.0f;
	float spread = m_bSilenced ? (moving ? 0.25f : 0.15f) : (moving ? 0.225f : 0.10f);
	if (airborne) spread = m_bSilenced ? 1.3f : 1.2f;
	USPFire(spread * (1.0f - m_flAccuracy));
}

void CUSPWeaponContext::USPFire(float spread)
{
	if (m_iClip <= 0) { if (m_fFireOnEmpty) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f); } return; }
	const float now = m_pLayer->GetTime();
	if (m_flLastFire > 0.0f) { m_flAccuracy -= (0.3f - (now - m_flLastFire)) * 0.275f; if (m_flAccuracy > 0.92f) m_flAccuracy = 0.92f; if (m_flAccuracy < 0.6f) m_flAccuracy = 0.6f; }
	m_flLastFire = now; --m_iClip;
	SendWeaponAnim(m_bSilenced ? (m_iClip ? USP_SHOOT1 : USP_SHOOT_EMPTY) : (m_iClip ? USP_UNSIL_SHOOT1 : USP_UNSIL_SHOOT_EMPTY), 0);
	Vector src = m_pLayer->GetGunPosition();
#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->SetAnimation(PLAYER_ATTACK1);
	player->m_iWeaponVolume = m_bSilenced ? QUIET_GUN_VOLUME : NORMAL_GUN_VOLUME; player->m_iWeaponFlash = m_bSilenced ? DIM_GUN_FLASH : NORMAL_GUN_FLASH;
	if (!m_bSilenced)
	{
		player->pev->effects |= EF_MUZZLEFLASH;
	}
#endif
	matrix3x3 aim = m_pLayer->GetCameraOrientation();
	Vector dir = m_pLayer->FireBullets(1, src, aim, 4096, spread, BULLET_PLAYER_45ACP, m_pLayer->GetRandomSeed());
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.20f); m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	WeaponEventParams p{}; p.flags = WeaponEventFlags::NotHost; p.eventindex = m_usFireUSP; p.origin = src; p.angles = aim.GetAngles(); p.fparam1 = dir.x; p.fparam2 = dir.y; p.bparam1 = m_iClip == 0; p.bparam2 = m_bSilenced;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(p);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 2.0f;
}

void CUSPWeaponContext::Reload()
{
	if (DefaultReload(USP_MAX_CLIP, m_bSilenced ? USP_RELOAD : USP_UNSIL_RELOAD, 3.1f, 0))
	{
		m_flAccuracy = 0.92f;
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 190.0f;
#endif
	}
}

void CUSPWeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 250.0f;
#endif
	if (m_iClip) { SendWeaponAnim(m_bSilenced ? USP_IDLE : USP_UNSIL_IDLE, 0); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 60.0f; }
}
