#include "weapon_m4.h"
#include "user_messages.h"
#include "weapon_layer.h"
#include "weapons/m4.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS(weapon_m4, CM4);

CM4::CM4()
{
	auto layerImpl = std::make_unique<CServerWeaponLayerImpl>(this);
	auto contextImpl = std::make_unique<CM4WeaponContext>(std::move(layerImpl));
	m_pWeaponContext = std::move(contextImpl);
}

void CM4::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(M4_CLASSNAME));
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/m4/w_m4.mdl");
	FallInit(); // get ready to fall down.
}

void CM4::Precache()
{
	PRECACHE_MODEL("models/weapon/m4/v_m4.mdl");
	PRECACHE_MODEL("models/weapon/m4/w_m4.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_SOUND("weapons/M4/m4-1.wav");// H to the K
	PRECACHE_SOUND("weapons/M4/m4-2.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");
}

int CM4::AddToPlayer(CBasePlayer *pPlayer)
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