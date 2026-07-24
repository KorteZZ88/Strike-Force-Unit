/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "weapon_mp5.h"
#include "user_messages.h"
#include "weapon_layer.h"
#include "weapons/mp5.h"
#include "server_weapon_layer_impl.h"

LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 );
LINK_ENTITY_TO_CLASS( weapon_9mmAR, CMP5 );

CMP5::CMP5()
{
	auto layerImpl = std::make_unique<CServerWeaponLayerImpl>(this);
	auto contextImpl = std::make_unique<CMP5WeaponContext>(std::move(layerImpl));
	m_pWeaponContext = std::move(contextImpl);
}

void CMP5::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(MP5_CLASSNAME));
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/mp5/w_mp5.mdl");
	FallInit(); // get ready to fall down.
}

void CMP5::Precache()
{
	PRECACHE_MODEL("models/weapon/mp5/v_mp5.mdl");
	PRECACHE_MODEL("models/weapon/mp5/w_mp5.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/MP-5/mp5-1.wav");// H to the K
	PRECACHE_SOUND("weapons/MP-5/mp5-2.wav");// H to the K
	PRECACHE_SOUND("weapons/hks3.wav");// H to the K

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");
}

int CMP5::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		return TRUE;
	}
	return FALSE;
}
