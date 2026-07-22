#include "weapon_usp.h"
#include "weapon_layer.h"
#include "weapons/usp.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS(weapon_usp, CUSP);

CUSP::CUSP()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CUSPWeaponContext>(std::move(layer));
}

void CUSP::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(USP_CLASSNAME)); Precache();
	SET_MODEL(edict(), "models/weapon/USP/w_usp.mdl"); FallInit();
}

void CUSP::Precache()
{
	PRECACHE_MODEL("models/weapon/USP/v_usp.mdl"); PRECACHE_MODEL("models/weapon/USP/p_usp.mdl"); PRECACHE_MODEL("models/weapon/USP/w_usp.mdl");
	PRECACHE_SOUND("weapons/USP/usp1.wav"); PRECACHE_SOUND("weapons/USP/usp2.wav"); PRECACHE_SOUND("weapons/USP/usp_unsil-1.wav");
	PRECACHE_SOUND("weapons/USP/usp_clipout.wav"); PRECACHE_SOUND("weapons/USP/usp_clipin.wav"); PRECACHE_SOUND("weapons/USP/usp_silencer_on.wav"); PRECACHE_SOUND("weapons/USP/usp_silencer_off.wav"); PRECACHE_SOUND("weapons/USP/usp_sliderelease.wav"); PRECACHE_SOUND("weapons/USP/usp_slideback.wav");
}
