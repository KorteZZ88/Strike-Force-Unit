#include "weapon_bomb.h"
#include "server_weapon_layer_impl.h"
#include "weapons/bomb.h"
LINK_ENTITY_TO_CLASS(weapon_bomb, CBombWeapon);
CBombWeapon::CBombWeapon() { auto layer = std::make_unique<CServerWeaponLayerImpl>(this); m_pWeaponContext = std::make_unique<CBombWeaponContext>(std::move(layer)); }
void CBombWeapon::Spawn() { Precache(); SET_MODEL(edict(), "models/weapon/Bomb/w_c4.mdl"); FallInit(); }
void CBombWeapon::Precache()
{
	PRECACHE_MODEL("models/weapon/Bomb/v_c4.mdl");
	PRECACHE_MODEL("models/weapon/Bomb/p_c4.mdl");
	PRECACHE_MODEL("models/weapon/Bomb/w_c4.mdl");
}
