#include "weapon_m60.h"
#include "weapons/m60.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_m60, CM60);

CM60::CM60()
{
	auto layer = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CM60WeaponContext>(std::move(layer));
}

void CM60::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(M60_CLASSNAME));
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/M60/w_m60.mdl");
	FallInit();
}

void CM60::Precache()
{
	PRECACHE_MODEL("models/weapon/M60/v_m60.mdl");
	PRECACHE_MODEL("models/weapon/M60/p_m60.mdl");
	PRECACHE_MODEL("models/weapon/M60/w_m60.mdl");
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/M60/m60-1.wav");
	PRECACHE_SOUND("weapons/M60/m60-2.wav");
	PrecacheViewModelSounds("models/weapon/M60/v_m60.mdl");
}

int CM60::AddToPlayer(CBasePlayer *player) { return CBasePlayerWeapon::AddToPlayer(player); }
