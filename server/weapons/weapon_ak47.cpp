#include "weapon_ak47.h"
#include "user_messages.h"
#include "weapons/ak47.h"
#include "server_weapon_layer_impl.h"
LINK_ENTITY_TO_CLASS(weapon_ak47, CAK47);
CAK47::CAK47() { auto layer = std::make_unique<CServerWeaponLayerImpl>(this); m_pWeaponContext = std::make_unique<CAK47WeaponContext>(std::move(layer)); }
void CAK47::Spawn() { pev->classname = MAKE_STRING(CLASSNAME_STR(AK47_CLASSNAME)); Precache(); SET_MODEL(ENT(pev), "models/weapon/AK-47/w_ak47.mdl"); FallInit(); }
void CAK47::Precache()
{
	PRECACHE_MODEL("models/weapon/AK-47/v_ak47.mdl"); PRECACHE_MODEL("models/weapon/AK-47/w_ak47.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl"); PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/AK-47/ak47-1.wav"); PRECACHE_SOUND("weapons/AK-47/ak47_magout.wav");
	PRECACHE_SOUND("weapons/AK-47/ak47_magin.wav"); PRECACHE_SOUND("weapons/AK-47/ak47_boltpull.wav");
}
int CAK47::AddToPlayer(CBasePlayer *player)
{
	if (!CBasePlayerWeapon::AddToPlayer(player)) return FALSE;
	MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, player->pev); WRITE_BYTE(m_pWeaponContext->m_iId); MESSAGE_END(); return TRUE;
}
