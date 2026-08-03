#include "weapon_bizon.h"
#include "weapon_layer.h"
#include "weapons/bizon.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_bizon, CBizon);

CBizon::CBizon()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CBizonWeaponContext>(std::move(layer));
}

void CBizon::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(BIZON_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/Bizon/w_bizon.mdl");
	FallInit();
}

void CBizon::Precache()
{
	PRECACHE_MODEL("models/weapon/Bizon/v_bizon.mdl");
	PRECACHE_MODEL("models/weapon/Bizon/p_bizon.mdl");
	PRECACHE_MODEL("models/weapon/Bizon/w_bizon.mdl");
	PrecacheViewModelSounds("models/weapon/Bizon/v_bizon.mdl");
	PRECACHE_SOUND("weapons/Bizon/bizon-1.wav");
}
