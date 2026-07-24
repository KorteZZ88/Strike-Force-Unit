#include "flashbang.h"
#include "const.h"
#include <utility>

#ifndef CLIENT_DLL
#include "weapon_flashbang.h"
#include "flashbang_grenade.h"
#endif

enum flashbang_e
{
	FLASHBANG_IDLE = 0,
	FLASHBANG_PINPULL,
	FLASHBANG_THROW,
	FLASHBANG_DRAW
};

constexpr float FLASHBANG_PINPULL_TIME = 31.0f / 30.0f;
constexpr float FLASHBANG_THROW_TIME = 31.0f / 30.0f;

CFlashbangWeaponContext::CFlashbangWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) :
	CBaseWeaponContext(std::move(layer)), m_flStartThrow(0.0f), m_flReleaseThrow(0.0f), m_bWeakThrow(false), m_bQueueNextThrow(false)
{
	m_iId = WEAPON_FLASHBANG;
	m_iDefaultAmmo = FLASHBANG_DEFAULT_GIVE;
}

int CFlashbangWeaponContext::GetItemInfo(ItemInfo* p) const
{
	p->pszName = CLASSNAME_STR(FLASHBANG_CLASSNAME);
	p->pszAmmo1 = "Flashbang";
	p->iMaxAmmo1 = FLASHBANG_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	// Grenade/explosive HUD slot (zero-based slot 3), with a position not shared by satchel.
	p->iPosition = 5;
	p->iId = m_iId;
	p->iWeight = FLASHBANG_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	return 1;
}

bool CFlashbangWeaponContext::Deploy()
{
	m_flReleaseThrow = -1;
	m_bWeakThrow = false;
	m_bQueueNextThrow = false;
	return DefaultDeploy("models/weapon/flashbang/v_flashbang.mdl", "models/weapon/flashbang/w_flashbang.mdl", FLASHBANG_DRAW, "crowbar");
}

bool CFlashbangWeaponContext::CanHolster() { return m_flStartThrow == 0; }

void CFlashbangWeaponContext::Holster()
{
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
#ifndef CLIENT_DLL
	CFlashbang* weapon = static_cast<CFlashbang*>(m_pLayer->GetWeaponEntity());
	EMIT_SOUND(ENT(weapon->m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM);
#endif
}

void CFlashbangWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
	{
#ifndef CLIENT_DLL
		static_cast<CFlashbang*>(m_pLayer->GetWeaponEntity())->RetireWeapon();
#endif
		return;
	}
	if (!m_flStartThrow && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
	{
		m_bWeakThrow = false;
		m_flStartThrow = m_pLayer->GetTime();
		m_flReleaseThrow = 0;
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + FLASHBANG_PINPULL_TIME;
		SendWeaponAnim(FLASHBANG_PINPULL);
#ifndef CLIENT_DLL
		CFlashbang* weapon = static_cast<CFlashbang*>(m_pLayer->GetWeaponEntity());
		EMIT_SOUND(ENT(weapon->m_pPlayer->pev), CHAN_WEAPON, "weapons/flashbang/pinpull.wav", 1.0f, ATTN_NORM);
#endif
	}
}

void CFlashbangWeaponContext::SecondaryAttack()
{
	if (!m_flStartThrow && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
	{
		m_bWeakThrow = true;
		m_flStartThrow = m_pLayer->GetTime();
		m_flReleaseThrow = 0;
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + FLASHBANG_PINPULL_TIME;
		SendWeaponAnim(FLASHBANG_PINPULL);
#ifndef CLIENT_DLL
		CFlashbang* weapon = static_cast<CFlashbang*>(m_pLayer->GetWeaponEntity());
		EMIT_SOUND(ENT(weapon->m_pPlayer->pev), CHAN_WEAPON, "weapons/flashbang/pinpull.wav", 1.0f, ATTN_NORM);
#endif
	}
}

void CFlashbangWeaponContext::WeaponIdle()
{
	if (m_flReleaseThrow > 0 && !m_flStartThrow && m_pLayer->CheckPlayerButtonFlag(IN_ATTACK))
		m_bQueueNextThrow = true;
	if (m_flReleaseThrow == 0 && m_flStartThrow)
		m_flReleaseThrow = m_pLayer->GetTime();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;

	if (m_flStartThrow)
	{
		Vector angThrow = m_pLayer->GetViewAngles();
		angThrow.x = angThrow.x < 0 ? -10 + angThrow.x * (80.0f / 90.0f) : -10 + angThrow.x * (100.0f / 90.0f);
		float velocity = (90 - angThrow.x) * 6.5f;
		if (velocity > 1000) velocity = 1000;
		if (m_bWeakThrow) velocity *= 0.5f;
#ifndef CLIENT_DLL
		CFlashbang* weapon = static_cast<CFlashbang*>(m_pLayer->GetWeaponEntity());
		UTIL_MakeVectors(angThrow);
		const Vector source = weapon->m_pPlayer->pev->origin + weapon->m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 4;
		const Vector throwVelocity = gpGlobals->v_forward * velocity + weapon->m_pPlayer->pev->velocity;
		CFlashbangGrenade::ShootTimed(weapon->m_pPlayer->pev, source, throwVelocity, 2.0f);
		weapon->m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
		SendWeaponAnim(FLASHBANG_THROW);
		// Keep m_flReleaseThrow positive until the following idle cycle.  That
		// cycle draws the next grenade or, when this was the last one, restores
		// the weapon held before the flashbang.
		m_flStartThrow = 0;
		m_bWeakThrow = false;
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(FLASHBANG_THROW_TIME);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + FLASHBANG_THROW_TIME;
		m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) - 1);
		return;
	}

	if (m_flReleaseThrow > 0)
	{
		m_flStartThrow = 0;
		if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
		{
			if (m_bQueueNextThrow)
			{
				m_bQueueNextThrow = false;
				m_bWeakThrow = false;
				m_flStartThrow = m_pLayer->GetTime();
				m_flReleaseThrow = 0;
				SendWeaponAnim(FLASHBANG_PINPULL);
				m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + FLASHBANG_PINPULL_TIME;
				return;
			}
			SendWeaponAnim(FLASHBANG_DRAW);
		}
#ifndef CLIENT_DLL
		else
		{
			CFlashbang* weapon = static_cast<CFlashbang*>(m_pLayer->GetWeaponEntity());
			if (weapon->RestoreWeaponBeforeFlashbang())
			{
			}
			else if (weapon->m_pPlayer->m_pLastItem && weapon->m_pPlayer->m_pLastItem != weapon)
				weapon->m_pPlayer->SelectLastItem();
			else
				weapon->RetireWeapon();
			return;
		}
#endif
		m_flReleaseThrow = -1;
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
		return;
	}

	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
	{
		SendWeaponAnim(FLASHBANG_IDLE);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10.0f;
	}
}
