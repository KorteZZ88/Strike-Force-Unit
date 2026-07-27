#include "weapon_colt1911.h"
#include "weapon_layer.h"
#include "weapons/colt1911.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS(weapon_1911, CColt1911);

CColt1911::CColt1911()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CColt1911WeaponContext>(std::move(layer));
}

void CColt1911::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(COLT1911_CLASSNAME)); Precache();
	SET_MODEL(edict(), "models/weapon/1911/w_1911.mdl"); FallInit();
}

void CColt1911::Precache()
{
	PRECACHE_MODEL("models/weapon/1911/v_1911.mdl"); PRECACHE_MODEL("models/weapon/1911/p_1911.mdl"); PRECACHE_MODEL("models/weapon/1911/w_1911.mdl");
	PRECACHE_SOUND("weapons/1911/1911-1.wav"); PRECACHE_SOUND("weapons/1911/1911-2.wav");
	PRECACHE_SOUND("weapons/1911/1911_clipout.wav"); PRECACHE_SOUND("weapons/1911/1911_clipin.wav");
	PRECACHE_SOUND("weapons/1911/1911_sliderelease.wav"); PRECACHE_SOUND("weapons/1911/1911_slideback.wav");
}
