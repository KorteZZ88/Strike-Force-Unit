#include "m72.h"

#ifndef CLIENT_DLL
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapon_m72.h"
#include "m72_rocket.h"
#endif

CM72WeaponContext::CM72WeaponContext(std::unique_ptr<IWeaponLayer>&& layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_M72;
	m_iDefaultAmmo = 1;
}

int CM72WeaponContext::GetItemInfo(ItemInfo* p) const
{
	p->pszName = CLASSNAME_STR(M72_CLASSNAME);
	p->pszAmmo1 = "M72Rocket";
	p->iMaxAmmo1 = 1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iId = m_iId;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = M72_WEIGHT;
	return 1;
}

bool CM72WeaponContext::Deploy()
{
	if (m_bSpent)
		return false;

	const bool deployed = DefaultDeploy("models/weapon/m72/v_law.mdl", "models/weapon/m72/p_law.mdl", M72_DRAW, "rpg");
	if (deployed)
	{
		const float now = m_pLayer->GetWeaponTimeBase(UsePredicting());
		m_pLayer->SetPlayerNextAttackTime(now + M72_DRAW_TIME);
		m_flNextPrimaryAttack = now + M72_DRAW_TIME;
		m_flTimeWeaponIdle = now + M72_DRAW_TIME;
	}
	return deployed;
}

void CM72WeaponContext::Holster()
{
	CBaseWeaponContext::Holster();
}

void CM72WeaponContext::PrimaryAttack()
{
	if (m_bSpent || m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
		return;

	m_bSpent = true;
	m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType, 0);
	SendWeaponAnim(M72_SHOOT);

#ifndef CLIENT_DLL
	CM72* weapon = static_cast<CM72*>(m_pLayer->GetWeaponEntity());
	CBasePlayer* player = weapon->m_pPlayer;
	player->SetAnimation(PLAYER_ATTACK1);
	player->m_iWeaponVolume = LOUD_GUN_VOLUME;
	player->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	UTIL_MakeVectors(player->pev->v_angle);
	const Vector source = player->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 - gpGlobals->v_up * 8;
	CM72Rocket* rocket = CM72Rocket::Create(source, player->pev->v_angle, player);
	rocket->SetLocalVelocity(rocket->GetLocalVelocity() + gpGlobals->v_forward * DotProduct(player->GetAbsVelocity(), gpGlobals->v_forward));
	EMIT_SOUND(player->edict(), CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9f, ATTN_NORM);
#endif

	const float now = m_pLayer->GetWeaponTimeBase(UsePredicting());
	m_flNextPrimaryAttack = now + M72_SHOOT_TIME;
	m_flNextSecondaryAttack = now + M72_SHOOT_TIME;
	m_flTimeWeaponIdle = now + M72_SHOOT_TIME;
}

void CM72WeaponContext::WeaponIdle()
{
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;

	if (m_bSpent)
	{
#ifndef CLIENT_DLL
		static_cast<CM72*>(m_pLayer->GetWeaponEntity())->RetireSpentLauncher();
#else
		m_pLayer->DisablePlayerViewmodel();
#endif
		return;
	}

	SendWeaponAnim(M72_IDLE);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
}
