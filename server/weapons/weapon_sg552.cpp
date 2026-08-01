#include "weapon_sg552.h"
#include "weapons/sg552.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_sg552, CSG552);
CSG552::CSG552() { m_pWeaponContext = std::make_unique<CSG552WeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CSG552::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(SG552_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/SG552/w_sg552.mdl"); FallInit(); }
void CSG552::Precache()
{
	PRECACHE_MODEL("models/weapon/SG552/v_sg552.mdl"); PRECACHE_MODEL("models/weapon/SG552/p_sg552.mdl");
	PRECACHE_MODEL("models/weapon/SG552/w_sg552.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/SG552/sg552-1.wav"); PRECACHE_SOUND("weapons/SG552/sg552-2.wav");
	PRECACHE_SOUND("weapons/SG552/sg552_clipout.wav"); PRECACHE_SOUND("weapons/SG552/sg552_clipin.wav");
	PRECACHE_SOUND("weapons/SG552/sg552_boltpull.wav");
	PrecacheViewModelSounds("models/weapon/SG552/v_sg552.mdl");
}
int CSG552::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	ItemInfo info = {}; if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS) CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE; return TRUE;
}
