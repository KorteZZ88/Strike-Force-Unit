#include "weapon_aug.h"
#include "weapons/aug.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS(weapon_aug, CAUG);
CAUG::CAUG() { m_pWeaponContext = std::make_unique<CAUGWeaponContext>(std::make_unique<CServerWeaponLayerImpl>(this)); }
void CAUG::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(AUG_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/AUG/w_aug.mdl"); FallInit(); }
void CAUG::Precache()
{
	PRECACHE_MODEL("models/weapon/AUG/v_aug.mdl"); PRECACHE_MODEL("models/weapon/AUG/p_aug.mdl");
	PRECACHE_MODEL("models/weapon/AUG/w_aug.mdl");
	PRECACHE_SOUND("weapons/AUG/aug-1.wav"); PRECACHE_SOUND("weapons/AUG/aug-2.wav");
	PRECACHE_SOUND("weapons/AUG/aug_boltpull.wav"); PRECACHE_SOUND("weapons/AUG/aug_boltslap.wav");
	PRECACHE_SOUND("weapons/AUG/aug_clipin.wav"); PRECACHE_SOUND("weapons/AUG/aug_clipout.wav");
	PRECACHE_SOUND("weapons/AUG/aug_forearm.wav"); PRECACHE_SOUND("weapons/AUG/aug_safety.wav");
	PrecacheViewModelSounds("models/weapon/AUG/v_aug.mdl");
}
int CAUG::AddToPlayer(CBasePlayer* player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	ItemInfo info = {}; if (GetItemInfo(&info) && info.iId > 0 && info.iId < MAX_WEAPONS) CBaseWeaponContext::ItemInfoArray[info.iId] = info;
	player->m_fKnownItem = FALSE; return TRUE;
}
