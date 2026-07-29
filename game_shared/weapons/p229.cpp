#include "p229.h"
#include <utility>

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#endif

CP229WeaponContext::CP229WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) : CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_P229;
	m_iDefaultAmmo = P229_DEFAULT_GIVE;
	m_usFireP229 = m_pLayer->PrecacheEvent("events/p229.sc");
}

int CP229WeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(P229_CLASSNAME); p->pszAmmo1 = "357sig"; p->iMaxAmmo1 = P229_MAX_CLIP * P229_MAX_SPARE_MAGAZINES;
	p->pszAmmo2 = NULL; p->iMaxAmmo2 = -1; p->iMaxClip = P229_MAX_CLIP; p->iSlot = 1; p->iPosition = 3;
	p->iFlags = 0; p->iId = m_iId; p->iWeight = P229_WEIGHT; return 1;
}

bool CP229WeaponContext::Deploy()
{
	const bool deployed = DefaultDeploy("models/weapon/P229/v_p229.mdl", "models/p_9mmhandgun.mdl", P229_DRAW, "onehanded");
	if (deployed)
		m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 1.25f);
	return deployed;
}

void CP229WeaponContext::PrimaryAttack()
{
	if (m_iClip <= 0) { if (m_fFireOnEmpty) { PlayEmptySound(); m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f); } return; }
	--m_iClip;
	const int shootAnimation = m_iClip ? P229_SHOOT1 + (m_pLayer->GetRandomSeed() % 3) : P229_SHOOT_EMPTY;
	SendWeaponAnim(shootAnimation);
#ifndef CLIENT_DLL
	CBasePlayer *player = m_pLayer->GetWeaponEntity()->m_pPlayer; player->SetAnimation(PLAYER_ATTACK1);
	player->pev->effects |= EF_MUZZLEFLASH; player->m_iWeaponVolume = NORMAL_GUN_VOLUME; player->m_iWeaponFlash = NORMAL_GUN_FLASH;
#endif
	Vector src = m_pLayer->GetGunPosition(); matrix3x3 aim = m_pLayer->GetCameraOrientation();
	Vector dir = m_pLayer->FireBullets(1, src, aim, 8192, 0.015f, BULLET_PLAYER_9MM, m_pLayer->GetRandomSeed());
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(60.0f / 350.0f);
	WeaponEventParams p{}; p.flags = WeaponEventFlags::NotHost; p.eventindex = m_usFireP229; p.origin = src; p.angles = aim.GetAngles(); p.fparam1 = dir.x; p.fparam2 = dir.y; p.iparam1 = shootAnimation; p.bparam1 = m_iClip == 0;
	if (m_pLayer->ShouldRunFuncs()) m_pLayer->PlaybackWeaponEvent(p);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.0f;
}

void CP229WeaponContext::Reload()
{
	const int reloadClipSize = m_iClip > 0 ? P229_MAX_CLIP + 1 : P229_MAX_CLIP;
	if (m_iClip >= reloadClipSize) return;
	if (DefaultReload(reloadClipSize, P229_RELOAD, 1.8f))
	{
#ifndef CLIENT_DLL
		m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 190.0f;
#endif
	}
}

void CP229WeaponContext::WeaponIdle()
{
	ResetEmptySound(); if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting())) return;
#ifndef CLIENT_DLL
	m_pLayer->GetWeaponEntity()->m_pPlayer->pev->maxspeed = 250.0f;
#endif
	if (m_iClip) { SendWeaponAnim(P229_IDLE); m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 3.0f; }
}
