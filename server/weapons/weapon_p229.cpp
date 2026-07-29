#include "weapon_p229.h"
#include "weapon_layer.h"
#include "weapons/p229.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_p229, CP229);

CP229::CP229()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CP229WeaponContext>(std::move(layer));
}

void CP229::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(P229_CLASSNAME)); Precache();
	SET_MODEL(edict(), "models/weapon/P229/w_p229.mdl"); FallInit();
}

void CP229::Precache()
{
	PRECACHE_MODEL("models/weapon/P229/v_p229.mdl"); PRECACHE_MODEL("models/p_9mmhandgun.mdl"); PRECACHE_MODEL("models/weapon/P229/w_p229.mdl");
	PrecacheViewModelSounds("models/weapon/P229/v_p229.mdl");
	PRECACHE_MODEL("models/shell.mdl"); PRECACHE_SOUND("weapons/P229/p229-1.wav");
	PRECACHE_SOUND("weapons/P229/p229_clipin.wav"); PRECACHE_SOUND("weapons/P229/p229_clipout.wav");
	PRECACHE_SOUND("weapons/P229/p229_slidepull.wav"); PRECACHE_SOUND("weapons/P229/p229_sliderelease.wav");
}
