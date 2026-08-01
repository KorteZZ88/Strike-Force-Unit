#include "weapon_famas.h"
#include "weapons/famas.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_famas, CFamas);
CFamas::CFamas() { m_pWeaponContext = std::make_unique<CFamasWeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CFamas::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(FAMAS_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/Famas/w_famas.mdl"); FallInit(); }
void CFamas::Precache()
{
	PRECACHE_MODEL("models/weapon/Famas/v_famas.mdl"); PRECACHE_MODEL("models/weapon/Famas/p_famas.mdl");
	PRECACHE_MODEL("models/weapon/Famas/w_famas.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/Famas/famas-1.wav"); PRECACHE_SOUND("weapons/Famas/famas-2.wav");
	PRECACHE_SOUND("weapons/Famas/famas-burst.wav"); PRECACHE_SOUND("weapons/Famas/famas_clipin.wav");
	PRECACHE_SOUND("weapons/Famas/famas_clipout.wav"); PRECACHE_SOUND("weapons/Famas/famas_forearm.wav");
	PrecacheViewModelSounds("models/weapon/Famas/v_famas.mdl");
}
int CFamas::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	ItemInfo info = {}; if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS) CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE; return TRUE;
}
