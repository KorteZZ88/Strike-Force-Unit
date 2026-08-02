#include "weapon_sg550.h"
#include "weapons/sg550.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_sg550, CSG550);
CSG550::CSG550() { m_pWeaponContext = std::make_unique<CSG550WeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CSG550::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(SG550_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/SG550/w_sg550.mdl"); FallInit(); }
void CSG550::Precache()
{
	PRECACHE_MODEL("models/weapon/SG550/v_sg550.mdl"); PRECACHE_MODEL("models/weapon/SG550/p_sg550.mdl");
	PRECACHE_MODEL("models/weapon/SG550/w_sg550.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/SG550/sg550-1.wav");
	PrecacheViewModelSounds("models/weapon/SG550/v_sg550.mdl");
}
int CSG550::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	ItemInfo info{}; if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS) CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE; return TRUE;
}
