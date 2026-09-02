#include "colt1911.h"
#include "usp.h"
#include <utility>

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#endif

CColt1911WeaponContext::CColt1911WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_COLT1911;
	m_iDefaultAmmo = COLT1911_DEFAULT_GIVE;
	m_usFire = m_pLayer->PrecacheEvent("events/1911.sc");
}

int CColt1911WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(COLT1911_CLASSNAME); p->pszAmmo1 = "45acp"; p->iMaxAmmo1 = COLT1911_MAX_CLIP * COLT1911_MAX_SPARE_MAGAZINES;
	p->pszAmmo2 = NULL; p->iMaxAmmo2 = -1; p->iMaxClip = COLT1911_MAX_CLIP; p->iSlot = 1; p->iPosition = 2;
	p->iFlags = 0; p->iId = m_iId; p->iWeight = COLT1911_WEIGHT; return 1;
}

bool CColt1911WeaponContext::Deploy()
{
	m_flAccuracy = 0.92f;
	m_pLayer->SetWeaponBodygroup(0);
	return DefaultDeploy("models/weapon/1911/v_1911.mdl", "models/weapon/1911/p_1911.mdl", USP_DRAW, "onehanded", 0);
}

void CColt1911WeaponContext::SecondaryAttack() {}

void CColt1911WeaponContext::PrimaryAttack()
{
	Fire(GetCs16PistolSpread(Cs16PistolProfile::USP, false));
}

void CColt1911WeaponContext::Fire(float spread)
{
	if (m_iClip <= 0) { if (m_fFireOnEmpty) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f); } return; }
	const float now = m_pLayer->GetTime();
	if (m_flLastFire > 0.0f) { m_flAccuracy -= (0.3f - (now - m_flLastFire)) * 0.275f; if (m_flAccuracy > 0.92f) m_flAccuracy = 0.92f; if (m_flAccuracy < 0.6f) m_flAccuracy = 0.6f; }
	m_flLastFire = now; --m_iClip;
	SendWeaponAnim(m_iClip ? USP_SHOOT1 : USP_SHOOT_EMPTY, 0);
	Vector src = m_pLayer->GetGunPosition();
#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->SetAnimation(PLAYER_ATTACK1);
	player->m_iWeaponVolume = QUIET_GUN_VOLUME; player->m_iWeaponFlash = DIM_GUN_FLASH;
#endif
	matrix3x3 aim = m_pLayer->GetCameraOrientation();
	Vector dir = m_pLayer->FireBullets(1, src, aim, 4096, spread, BULLET_PLAYER_45ACP, m_pLayer->GetRandomSeed());
	KickBack(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(ConfigFireInterval(0.20f)); m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	WeaponEventParams p{}; p.flags = WeaponEventFlags::NotHost; p.eventindex = m_usFire; p.origin = src; p.angles = aim.GetAngles(); p.fparam1 = dir.x; p.fparam2 = dir.y; p.bparam1 = m_iClip == 0; p.bparam2 = true;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(p);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 2.0f;
}

void CColt1911WeaponContext::Reload()
{
	if (DefaultReload(COLT1911_MAX_CLIP + (m_iClip > 0 ? 1 : 0), USP_RELOAD, 3.1f, 0)) { m_flAccuracy = 0.92f; m_flCs16PistolAccuracy = -1.0f; }
}

void CColt1911WeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	if (m_iClip) { SendWeaponAnim(USP_IDLE, 0); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 60.0f; }
}
