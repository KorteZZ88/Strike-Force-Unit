#include "weapon_fiveseven.h"
#include "weapon_layer.h"
#include "weapons/fiveseven.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_fiveseven, CFiveSeven);

CFiveSeven::CFiveSeven()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CFiveSevenWeaponContext>(std::move(layer));
}

void CFiveSeven::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(FIVESEVEN_CLASSNAME)); Precache();
	SET_MODEL(edict(), "models/weapon/FiveSeven/w_fiveseven.mdl"); FallInit();
}

void CFiveSeven::Precache()
{
	PRECACHE_MODEL("models/weapon/FiveSeven/v_fiveseven.mdl"); PRECACHE_MODEL("models/weapon/FiveSeven/p_fiveseven.mdl"); PRECACHE_MODEL("models/weapon/FiveSeven/w_fiveseven.mdl");
	PrecacheViewModelSounds("models/weapon/FiveSeven/v_fiveseven.mdl");
	PRECACHE_MODEL("models/shell.mdl"); PRECACHE_SOUND("weapons/FiveSeven/fiveseven-1.wav"); PRECACHE_SOUND("weapons/FiveSeven/fiveseven-2.wav");
	PRECACHE_SOUND("weapons/FiveSeven/57_clipin.wav"); PRECACHE_SOUND("weapons/FiveSeven/57_clipout.wav");
	PRECACHE_SOUND("weapons/FiveSeven/57_deploy.wav"); PRECACHE_SOUND("weapons/FiveSeven/57_release.wav");
}
