#include "weapon_mp5sd.h"
#include "weapon_layer.h"
#include "weapons/mp5sd.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS(weapon_mp5sd, CMP5SD);

CMP5SD::CMP5SD()
{
	auto layerImpl = std::make_unique<CServerWeaponLayerImpl>(this);
	m_pWeaponContext = std::make_unique<CMP5SDWeaponContext>(std::move(layerImpl));
}

void CMP5SD::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(MP5SD_CLASSNAME));
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/MP-5SD/w_mp5.mdl");
	FallInit();
}

void CMP5SD::Precache()
{
	PRECACHE_MODEL("models/weapon/MP-5SD/v_mp5sd.mdl");
	PRECACHE_MODEL("models/weapon/MP-5SD/p_mp5sd.mdl");
	PRECACHE_MODEL("models/weapon/MP-5SD/w_mp5.mdl");
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/MP-5SD/mp5-1.wav");
	PRECACHE_SOUND("weapons/MP-5SD/mp5-boltpull.wav");
	PRECACHE_SOUND("weapons/MP-5SD/mp5-boltslap.wav");
	PRECACHE_SOUND("weapons/MP-5SD/mp5-clipin.wav");
	PRECACHE_SOUND("weapons/MP-5SD/mp5-clipout.wav");
	PRECACHE_SOUND("weapons/MP-5SD/mp5-draw.wav");
}

int CMP5SD::AddToPlayer(CBasePlayer *pPlayer)
{
	return CBasePlayerWeapon::AddToPlayer(pPlayer);
}
