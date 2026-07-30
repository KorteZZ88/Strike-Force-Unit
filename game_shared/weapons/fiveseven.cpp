#include "fiveseven.h"
#include <utility>

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#endif

CFiveSevenWeaponContext::CFiveSevenWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_FIVESEVEN;
	m_iDefaultAmmo = FIVESEVEN_DEFAULT_GIVE;
	m_usFireFiveSeven = m_pLayer->PrecacheEvent("events/fiveseven.sc");
}

int CFiveSevenWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(FIVESEVEN_CLASSNAME); p->pszAmmo1 = "5.7x28mm"; p->iMaxAmmo1 = FIVESEVEN_MAX_CLIP * FIVESEVEN_MAX_SPARE_MAGAZINES;
	p->pszAmmo2 = NULL; p->iMaxAmmo2 = -1; p->iMaxClip = FIVESEVEN_MAX_CLIP; p->iSlot = 1; p->iPosition = 4;
	p->iFlags = 0; p->iId = m_iId; p->iWeight = FIVESEVEN_WEIGHT; return 1;
}

bool CFiveSevenWeaponContext::Deploy()
{
	const bool deployed = DefaultDeploy("models/weapon/FiveSeven/v_fiveseven.mdl", "models/weapon/FiveSeven/p_fiveseven.mdl", FIVESEVEN_DRAW, "onehanded");
	if (deployed) m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.25f);
	return deployed;
}

void CFiveSevenWeaponContext::PrimaryAttack()
{
	if (m_iClip <= 0) { if (m_fFireOnEmpty) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f); } return; }
	--m_iClip;
	const int shootAnimation = m_iClip ? FIVESEVEN_SHOOT1 + (m_pLayer->GetRandomSeed() % 2) : FIVESEVEN_SHOOT_LAST;
	SendWeaponAnim(shootAnimation);
#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->SetAnimation(PLAYER_ATTACK1);
	player->pev->effects |= EF_MUZZLEFLASH; player->m_iWeaponVolume = NORMAL_GUN_VOLUME; player->m_iWeaponFlash = NORMAL_GUN_FLASH;
#endif
	Vector src = m_pLayer->GetGunPosition(); matrix3x3 aim = m_pLayer->GetCameraOrientation();
	const float spread = GetCs16PistolSpread(Cs16PistolProfile::FiveSeven);
	Vector dir = m_pLayer->FireBullets(1, src, aim, 8192, spread, BULLET_PLAYER_9MM, m_pLayer->GetRandomSeed());
	KickBack(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(60.0f / 400.0f);
	WeaponEventParams p{}; p.flags = WeaponEventFlags::NotHost; p.eventindex = m_usFireFiveSeven; p.origin = src; p.angles = aim.GetAngles(); p.fparam1 = dir.x; p.fparam2 = dir.y; p.iparam1 = shootAnimation; p.bparam1 = m_iClip == 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(p);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.0f;
}

void CFiveSevenWeaponContext::Reload()
{
	const int reloadClipSize = m_iClip > 0 ? FIVESEVEN_MAX_CLIP + 1 : FIVESEVEN_MAX_CLIP;
	if (m_iClip >= reloadClipSize) return;
	if (DefaultReload(reloadClipSize, FIVESEVEN_RELOAD, 1.8f)) m_flCs16PistolAccuracy = -1.0f;
}

void CFiveSevenWeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
	if (m_iClip) { SendWeaponAnim(FIVESEVEN_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.0f; }
}
