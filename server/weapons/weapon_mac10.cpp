#include "weapon_mac10.h"
#include "weapon_layer.h"
#include "weapons/mac10.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_mac10, CMac10);

CMac10::CMac10()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CMac10WeaponContext>(std::move(layer));
}

void CMac10::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(MAC10_CLASSNAME));
	Precache();
	SET_MODEL(edict(), "models/weapon/Mac-10/w_mac10.mdl");
	FallInit();
}

void CMac10::Precache()
{
	PRECACHE_MODEL("models/weapon/Mac-10/v_mac10.mdl");
	PRECACHE_MODEL("models/weapon/Mac-10/p_mac10.mdl");
	PRECACHE_MODEL("models/weapon/Mac-10/w_mac10.mdl");
	PrecacheViewModelSounds("models/weapon/Mac-10/v_mac10.mdl");
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/Mac-10/mac10-1.wav");
	PRECACHE_SOUND("weapons/Mac-10/mac10_boltpull.wav");
	PRECACHE_SOUND("weapons/Mac-10/mac10_clipin.wav");
	PRECACHE_SOUND("weapons/Mac-10/mac10_clipout.wav");
}

int CMac10::AddToPlayer(CBasePlayer *player)
{
	return CBasePlayerWeapon::AddToPlayer(player);
}
