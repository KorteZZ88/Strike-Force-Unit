#include "smokegrenade.h"
#include <utility>
#ifndef CLIENT_DLL
#include "weapon_smokegrenade.h"
#include "smoke_grenade.h"
#endif

enum smokegrenade_e { SMOKE_IDLE, SMOKE_PINPULL, SMOKE_THROW, SMOKE_DRAW };
constexpr float SMOKEGRENADE_PINPULL_TIME = 50.0f / 41.0f;
constexpr float SMOKEGRENADE_THROW_TIME = 30.0f / 30.0f;

CSmokeGrenadeWeaponContext::CSmokeGrenadeWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) :
	CBaseWeaponContext(std::move(layer)), m_flStartThrow(0), m_flReleaseThrow(0), m_bWeakThrow(false)
{
	m_iId = WEAPON_SMOKEGRENADE;
	m_iDefaultAmmo = SMOKEGRENADE_DEFAULT_GIVE;
}

int CSmokeGrenadeWeaponContext::GetItemInfo(ItemInfo* p) const
{
	p->pszName = CLASSNAME_STR(SMOKEGRENADE_CLASSNAME);
	p->pszAmmo1 = "SmokeGrenade";
	p->iMaxAmmo1 = 1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 7;
	p->iId = m_iId;
	p->iWeight = SMOKEGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	return 1;
}

bool CSmokeGrenadeWeaponContext::Deploy()
{
	m_flReleaseThrow = -1;
	m_bWeakThrow = false;
	return DefaultDeploy("models/weapon/Gasgrenade/v_smokegrenade.mdl", "models/weapon/Gasgrenade/w_smokegrenade.mdl", SMOKE_DRAW, "crowbar");
}

bool CSmokeGrenadeWeaponContext::CanHolster() { return m_flStartThrow == 0; }

void CSmokeGrenadeWeaponContext::Holster()
{
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + .5f);
}

void CSmokeGrenadeWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) <= 0)
	{
#ifndef CLIENT_DLL
		static_cast<CSmokeGrenadeWeapon*>(m_pLayer->GetWeaponEntity())->RetireWeapon();
#endif
		return;
	}
	if (!m_flStartThrow)
	{
		m_bWeakThrow = false;
		m_flStartThrow = m_pLayer->GetTime();
		m_flReleaseThrow = 0;
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + SMOKEGRENADE_PINPULL_TIME;
		SendWeaponAnim(SMOKE_PINPULL);
#ifndef CLIENT_DLL
		CSmokeGrenadeWeapon* weapon = static_cast<CSmokeGrenadeWeapon*>(m_pLayer->GetWeaponEntity());
		EMIT_SOUND(ENT(weapon->m_pPlayer->pev), CHAN_WEAPON, "weapons/flashbang/pinpull.wav", 1.0f, ATTN_NORM);
#endif
	}
}

void CSmokeGrenadeWeaponContext::SecondaryAttack()
{
	if (!m_flStartThrow && m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
	{
		m_bWeakThrow = true;
		m_flStartThrow = m_pLayer->GetTime();
		m_flReleaseThrow = 0;
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + SMOKEGRENADE_PINPULL_TIME;
		SendWeaponAnim(SMOKE_PINPULL);
#ifndef CLIENT_DLL
		CSmokeGrenadeWeapon* weapon = static_cast<CSmokeGrenadeWeapon*>(m_pLayer->GetWeaponEntity());
		EMIT_SOUND(ENT(weapon->m_pPlayer->pev), CHAN_WEAPON, "weapons/flashbang/pinpull.wav", 1.0f, ATTN_NORM);
#endif
	}
}

void CSmokeGrenadeWeaponContext::WeaponIdle()
{
	if (m_flReleaseThrow == 0 && m_flStartThrow)
		m_flReleaseThrow = m_pLayer->GetTime();
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;

	if (m_flStartThrow)
	{
		Vector angles = m_pLayer->GetViewAngles();
		angles.x = angles.x < 0 ? -10 + angles.x * (80.f / 90.f) : -10 + angles.x * (100.f / 90.f);
		float velocity = (90 - angles.x) * 6.5f;
		if (velocity > 1000) velocity = 1000;
		if (m_bWeakThrow) velocity *= .5f;
#ifndef CLIENT_DLL
		CSmokeGrenadeWeapon* weapon = static_cast<CSmokeGrenadeWeapon*>(m_pLayer->GetWeaponEntity());
		UTIL_MakeVectors(angles);
		CSmokeGrenade::ShootTimed(weapon->m_pPlayer->pev,
			weapon->m_pPlayer->pev->origin + weapon->m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
			gpGlobals->v_forward * velocity + weapon->m_pPlayer->pev->velocity, 3.f);
		weapon->m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
		SendWeaponAnim(SMOKE_THROW);
		m_flStartThrow = 0;
		m_bWeakThrow = false;
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(SMOKEGRENADE_THROW_TIME);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + SMOKEGRENADE_THROW_TIME;
		m_pLayer->SetPlayerAmmo(m_iPrimaryAmmoType, m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) - 1);
		return;
	}

	if (m_flReleaseThrow > 0)
	{
		m_flStartThrow = 0;
		if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
			SendWeaponAnim(SMOKE_DRAW);
#ifndef CLIENT_DLL
		else
		{
			CSmokeGrenadeWeapon* weapon = static_cast<CSmokeGrenadeWeapon*>(m_pLayer->GetWeaponEntity());
			weapon->RetireWeapon();
			return;
		}
#endif
		m_flReleaseThrow = -1;
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10;
		return;
	}

	if (m_pLayer->GetPlayerAmmo(m_iPrimaryAmmoType) > 0)
	{
		SendWeaponAnim(SMOKE_IDLE);
		m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 10;
	}
}
