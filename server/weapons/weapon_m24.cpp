

#include "weapon_m24.h"
#include "user_messages.h"
#include "weapon_layer.h"
#include "weapons/m24.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS(weapon_m24, CM24);

CM24::CM24()
{
	auto layerImpl = std::make_unique<CServerWeaponLayerImpl>(this);
	auto contextImpl = std::make_unique<CM24WeaponContext>(std::move(layerImpl));
	m_pWeaponContext = std::move(contextImpl);
}

void CM24::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(M24_CLASSNAME));
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/mp5/w_mp5.mdl");
	FallInit(); // get ready to fall down.
}

void CM24::Precache()
{
	PRECACHE_MODEL("models/w_crossbow.mdl");
	PRECACHE_MODEL("models/weapon/m24/v_m24.mdl");
	PRECACHE_MODEL("models/weapon/mp5/w_mp5.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND("weapons/r700/R700-1.wav");
}

int CM24::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_pWeaponContext->m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}
