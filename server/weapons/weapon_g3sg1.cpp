#include "weapon_g3sg1.h"
#include "weapons/g3sg1.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_g3sg1, CG3SG1);
CG3SG1::CG3SG1() { m_pWeaponContext = std::make_unique<CG3SG1WeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CG3SG1::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(G3SG1_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/G3SG1/w_g3sg1.mdl"); FallInit(); }
void CG3SG1::Precache()
{
	PRECACHE_MODEL("models/weapon/G3SG1/v_g3sg1.mdl"); PRECACHE_MODEL("models/weapon/G3SG1/p_g3sg1.mdl");
	PRECACHE_MODEL("models/weapon/G3SG1/w_g3sg1.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/G3SG1/g3sg1-1.wav"); PRECACHE_SOUND("weapons/G3SG1/g3sg1-2.wav");
	PrecacheViewModelSounds("models/weapon/G3SG1/v_g3sg1.mdl");
}
int CG3SG1::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	ItemInfo info{}; if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS) CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE; return TRUE;
}
