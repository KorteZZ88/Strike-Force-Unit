#include "weapon_smokegrenade.h"
#include "weapon_layer.h"
#include "weapons/smokegrenade.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS(weapon_smokegrenade, CSmokeGrenadeWeapon);

CSmokeGrenadeWeapon::CSmokeGrenadeWeapon()
{
	m_pWeaponContext = std::make_unique<CSmokeGrenadeWeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this));
}

void CSmokeGrenadeWeapon::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/Gasgrenade/w_smokegrenade.mdl");
	FallInit();
}

void CSmokeGrenadeWeapon::Precache()
{
	PRECACHE_MODEL("models/weapon/Gasgrenade/v_smokegrenade.mdl");
	PRECACHE_MODEL("models/weapon/Gasgrenade/w_smokegrenade.mdl");
	PRECACHE_MODEL("sprites/smoke_puff.spr");
	PRECACHE_SOUND("weapons/flashbang/pinpull.wav");
	PRECACHE_SOUND("weapons/flashbang/flashbang_hit.wav");
}
