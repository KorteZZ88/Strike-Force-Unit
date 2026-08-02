#include "weapon_awp.h"
#include "weapons/awp.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_awp, CAWP);
CAWP::CAWP() { m_pWeaponContext = std::make_unique<CAWPWeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CAWP::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(AWP_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/AWP/w_awp.mdl"); FallInit(); }
void CAWP::Precache()
{
	PRECACHE_MODEL("models/weapon/AWP/v_awp.mdl"); PRECACHE_MODEL("models/weapon/AWP/w_awp.mdl"); PRECACHE_MODEL("models/p_9mmAR.mdl");
	PRECACHE_MODEL("models/shell.mdl"); PRECACHE_SOUND("weapons/AWP/awp1.wav"); PRECACHE_SOUND("weapons/AWP/awp2.wav");
	PrecacheViewModelSounds("models/weapon/AWP/v_awp.mdl");
}
int CAWP::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	ItemInfo info{}; if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS) CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE; return TRUE;
}
