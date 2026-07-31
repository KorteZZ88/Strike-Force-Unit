#include "weapon_p90.h"
#include "weapon_layer.h"
#include "weapons/p90.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_p90, CP90);

CP90::CP90()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CP90WeaponContext>(std::move(layer));
}

void CP90::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(P90_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/P90/w_p90.mdl");
	FallInit();
}

void CP90::Precache()
{
	PRECACHE_MODEL("models/weapon/P90/v_p90.mdl");
	PRECACHE_MODEL("models/weapon/P90/p_p90.mdl");
	PRECACHE_MODEL("models/weapon/P90/w_p90.mdl");
	PrecacheViewModelSounds("models/weapon/P90/v_p90.mdl");
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/P90/p90-1.wav");
}
