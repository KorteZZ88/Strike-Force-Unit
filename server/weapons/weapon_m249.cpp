#include "weapon_m249.h"
#include "weapons/m249.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_m249, CM249);
CM249::CM249() { auto layer = std::make_unique<CServerWeaponLayerImpl>(this); m_pWeaponContext = std::make_unique<CM249WeaponContext>(std::move(layer)); }
void CM249::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(M249_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/M249/w_m249.mdl"); FallInit(); }
void CM249::Precache()
{
	PRECACHE_MODEL("models/weapon/M249/v_m249_mirror.mdl"); PRECACHE_MODEL("models/weapon/M249/p_m249.mdl"); PRECACHE_MODEL("models/weapon/M249/w_m249.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/M249/m249-1.wav"); PRECACHE_SOUND("weapons/M249/m249-2.wav"); PrecacheViewModelSounds("models/weapon/M249/v_m249_mirror.mdl");
}
int CM249::AddToPlayer(CBasePlayer* player) { return CBasePlayerWeapon::AddToPlayer(player); }
