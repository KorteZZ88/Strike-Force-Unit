#include "weapon_flashbang.h"
#include "weapon_layer.h"
#include "weapons/flashbang.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS(weapon_flashbang, CFlashbang);

BEGIN_DATADESC(CFlashbang)
	DEFINE_FIELD(m_pWeaponBeforeFlashbang, FIELD_CLASSPTR),
END_DATADESC()

CFlashbang::CFlashbang() : m_pWeaponBeforeFlashbang(NULL)
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CFlashbangWeaponContext>(std::move(layer));
}

void CFlashbang::RememberWeaponBeforeFlashbang(CBasePlayerItem* weapon)
{
	if (weapon && weapon != this)
		m_pWeaponBeforeFlashbang = weapon;
}

BOOL CFlashbang::RestoreWeaponBeforeFlashbang()
{
	if (!m_pPlayer || !m_pWeaponBeforeFlashbang ||
		m_pWeaponBeforeFlashbang == this || !m_pPlayer->HasPlayerItem(m_pWeaponBeforeFlashbang))
		return FALSE;

	CBasePlayerItem* previous = m_pWeaponBeforeFlashbang;
	m_pWeaponBeforeFlashbang = NULL;
	return m_pPlayer->SwitchWeapon(previous);
}

void CFlashbang::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/flashbang/w_flashbang.mdl");
	FallInit();
}

void CFlashbang::Precache()
{
	PRECACHE_MODEL("models/weapon/flashbang/v_flashbang.mdl");
	PRECACHE_MODEL("models/weapon/flashbang/w_flashbang.mdl");
	PRECACHE_SOUND("weapons/flashbang/flashbang.wav");
	PRECACHE_SOUND("weapons/flashbang/flashbang_hit.wav");
}
