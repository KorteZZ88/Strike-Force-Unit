#include "weapon_gasgrenade.h"
#include "weapon_layer.h"
#include "weapons/gasgrenade.h"
#include "server_weapon_layer_impl.h"
LINK_ENTITY_TO_CLASS(weapon_gasgrenade, CGasGrenadeWeapon);
CGasGrenadeWeapon::CGasGrenadeWeapon() { m_pWeaponContext=std::make_unique<CGasGrenadeWeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CGasGrenadeWeapon::Spawn() { Precache(); SET_MODEL(ENT(pev),"models/weapon/flashbang/w_flashbang.mdl"); FallInit(); }
void CGasGrenadeWeapon::Precache() { PRECACHE_MODEL("models/weapon/flashbang/v_flashbang.mdl"); PRECACHE_MODEL("models/weapon/flashbang/w_flashbang.mdl"); PRECACHE_MODEL("sprites/poison.spr"); PRECACHE_SOUND("weapons/flashbang/flashbang_hit.wav"); }
