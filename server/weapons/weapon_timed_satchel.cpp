#include "weapon_timed_satchel.h"
#include "timed_satchel_entities.h"
#include "server_weapon_layer_impl.h"
#include "weapons/timed_satchel.h"

LINK_ENTITY_TO_CLASS(weapon_c4, CTimedSatchel);

#define DEFINE_C4WEAPON_FIELD(x, ft) \
	DEFINE_CUSTOM_FIELD(x, ft, [](CBaseEntity *entity, void *data, size_t size) { \
		auto *weapon = static_cast<CBasePlayerWeapon*>(entity); \
		auto *context = static_cast<CTimedSatchelWeaponContext*>(weapon->m_pWeaponContext.get()); \
		std::memcpy(data, &context->x, size); \
	}, [](CBaseEntity *entity, const void *data, size_t size) { \
		auto *weapon = static_cast<CBasePlayerWeapon*>(entity); \
		auto *context = static_cast<CTimedSatchelWeaponContext*>(weapon->m_pWeaponContext.get()); \
		std::memcpy(&context->x, data, size); \
	})

BEGIN_DATADESC(CTimedSatchel)
	DEFINE_C4WEAPON_FIELD(m_iTimerSeconds, FIELD_INTEGER),
END_DATADESC()

CTimedSatchel::CTimedSatchel()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CTimedSatchelWeaponContext>(std::move(layer));
}

void CTimedSatchel::Spawn()
{
	Precache();
	SET_MODEL(edict(), "models/w_satchel.mdl");
	FallInit();
}

void CTimedSatchel::Precache()
{
	PRECACHE_MODEL("models/v_satchel.mdl");
	PRECACHE_MODEL("models/w_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel.mdl");
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	UTIL_PrecacheOther("timed_satchel_bomb");
	UTIL_PrecacheOther("timed_satchel_preview");
}

void CTimedSatchel::CreatePreview()
{
	RemovePreview();
	m_hPreview = CBaseEntity::Create("timed_satchel_preview", m_pPlayer->GetGunPosition(), g_vecZero, m_pPlayer->edict());
}

void CTimedSatchel::RemovePreview()
{
	if (m_hPreview != NULL)
		UTIL_Remove((CBaseEntity *)m_hPreview);
	m_hPreview = NULL;
}

CTimedSatchelPreview *CTimedSatchel::GetPreview()
{
	return !m_hPreview.Get() ? nullptr : static_cast<CTimedSatchelPreview*>(m_hPreview.GetPointer());
}
