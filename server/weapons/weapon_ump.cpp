#include "weapon_ump.h"
#include "weapon_layer.h"
#include "weapons/ump.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_ump, CUMP);

CUMP::CUMP()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CUMPWeaponContext>(std::move(layer));
}

void CUMP::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(UMP_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/UMP/w_ump45.mdl");
	FallInit();
}

void CUMP::Precache()
{
	PRECACHE_MODEL("models/weapon/UMP/v_ump45.mdl");
	PRECACHE_MODEL("models/weapon/UMP/p_ump45.mdl");
	PRECACHE_MODEL("models/weapon/UMP/w_ump45.mdl");
	PrecacheViewModelSounds("models/weapon/UMP/v_ump45.mdl");
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/UMP/ump45-1.wav");
	PRECACHE_SOUND("weapons/UMP/ump45-boltslap.wav");
	PRECACHE_SOUND("weapons/UMP/ump45-clipin.wav");
	PRECACHE_SOUND("weapons/UMP/ump45-clipout.wav");
	PRECACHE_SOUND("weapons/UMP/ump45-draw.wav");
}
