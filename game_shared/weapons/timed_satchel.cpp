#include "timed_satchel.h"
#include <utility>

#ifndef CLIENT_DLL
#include "weapon_timed_satchel.h"
#include "timed_satchel_entities.h"
#include "player.h"
#include "user_messages.h"
#endif

CTimedSatchelWeaponContext::CTimedSatchelWeaponContext(std::unique_ptr<IWeaponLayer>&& layer) :
	CBaseWeaponContext(std::move(layer))
{
	m_iId = WEAPON_C4;
	m_iDefaultAmmo = C4_DEFAULT_GIVE;
	m_iTimerSeconds = 10;
}

int CTimedSatchelWeaponContext::GetItemInfo(ItemInfo *p) const
{
	p->pszName = CLASSNAME_STR(C4_CLASSNAME);
	p->pszAmmo1 = "C4";
	p->iMaxAmmo1 = C4_MAX_CARRY;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId;
	p->iWeight = C4_WEIGHT;
	return 1;
}

bool CTimedSatchelWeaponContext::Deploy()
{
	const bool deployed = DefaultDeploy("models/v_satchel.mdl", "models/p_satchel.mdl", C4_DRAW, "trip");
#ifndef CLIENT_DLL
	if (deployed)
		static_cast<CTimedSatchel*>(m_pLayer->GetWeaponEntity())->CreatePreview();
#endif
	return deployed;
}

void CTimedSatchelWeaponContext::Holster()
{
	m_pLayer->SetPlayerNextAttackTime(m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f);
	SendWeaponAnim(C4_PLACE);
#ifndef CLIENT_DLL
	CTimedSatchel *weapon = static_cast<CTimedSatchel*>(m_pLayer->GetWeaponEntity());
	weapon->RemovePreview();
	EMIT_SOUND(ENT(weapon->m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM);
#endif
}

void CTimedSatchelWeaponContext::PrimaryAttack()
{
	if (m_pLayer->GetPlayerAmmo(PrimaryAmmoIndex()) <= 0)
		return;

#ifndef CLIENT_DLL
	CTimedSatchel *weapon = static_cast<CTimedSatchel*>(m_pLayer->GetWeaponEntity());
	CTimedSatchelPreview *preview = weapon->GetPreview();
	if (!preview || !preview->CanPlace())
	{
		m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.2f);
		return;
	}

	preview->PlaceBomb(m_iTimerSeconds);
	weapon->m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	MESSAGE_BEGIN(MSG_ONE, gmsgPickupHint, NULL, weapon->m_pPlayer->pev);
		WRITE_STRING(UTIL_VarArgs("Timer: %ds", m_iTimerSeconds));
	MESSAGE_END();
#endif

	SendWeaponAnim(C4_PLACE);
	m_pLayer->SetPlayerAmmo(PrimaryAmmoIndex(), m_pLayer->GetPlayerAmmo(PrimaryAmmoIndex()) - 1);
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.5f);
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.5f;

#ifndef CLIENT_DLL
	weapon->RemovePreview();
	if (weapon->m_pPlayer->m_pLastItem && weapon->m_pPlayer->m_pLastItem != weapon)
		weapon->m_pPlayer->SelectLastItem();
	else
		weapon->RetireWeapon();
#endif
}

void CTimedSatchelWeaponContext::SecondaryAttack()
{
	m_iTimerSeconds = m_iTimerSeconds == 10 ? 30 : 10;
#ifndef CLIENT_DLL
	CTimedSatchel *weapon = static_cast<CTimedSatchel*>(m_pLayer->GetWeaponEntity());
	MESSAGE_BEGIN(MSG_ONE, gmsgPickupHint, NULL, weapon->m_pPlayer->pev);
		WRITE_STRING(UTIL_VarArgs("Timer: %ds", m_iTimerSeconds));
	MESSAGE_END();
#endif
	m_flNextPrimaryAttack = GetNextPrimaryAttackDelay(0.25f);
	m_flNextSecondaryAttack = m_pLayer->GetWeaponTimeBase(UsePredicting()) + 0.25f;
}

void CTimedSatchelWeaponContext::WeaponIdle()
{
	if (m_flTimeWeaponIdle > m_pLayer->GetWeaponTimeBase(UsePredicting()))
		return;

	if (m_pLayer->GetPlayerAmmo(PrimaryAmmoIndex()) <= 0)
{
#ifndef CLIENT_DLL
		static_cast<CTimedSatchel*>(m_pLayer->GetWeaponEntity())->RetireWeapon();
#endif
		return;
	}

	SendWeaponAnim(C4_FIDGET);
	m_flTimeWeaponIdle = m_pLayer->GetWeaponTimeBase(UsePredicting()) +
		m_pLayer->GetRandomFloat(m_pLayer->GetRandomSeed(), 10.0f, 15.0f);
}
