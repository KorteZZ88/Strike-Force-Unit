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

#include "weapon_m3.h"
#include "weapon_layer.h"
#include "weapons/m3.h"
#include "server_weapon_layer_impl.h"
#include "user_messages.h"

LINK_ENTITY_TO_CLASS( weapon_m3, CM3 );

CM3::CM3()
{
	auto layerImpl = std::make_unique<CServerWeaponLayerImpl>(this);
	auto contextImpl = std::make_unique<CM3WeaponContext>(std::move(layerImpl));
	m_pWeaponContext = std::move(contextImpl);
}

void CM3::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(M3_CLASSNAME)); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");
	FallInit(); // get ready to fall
}

void CM3::Precache()
{
	PRECACHE_MODEL("models/weapon/m3/v_m3.mdl");
	PRECACHE_MODEL("models/weapon/m3/w_m3.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");
	PRECACHE_MODEL("models/shotgunshell.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");              
	PRECACHE_SOUND("weapons/m3/m3-1.wav");	// shotgun
	PRECACHE_SOUND("weapons/sbarrel1.wav");	// shotgun
	PRECACHE_SOUND("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav");	// shotgun reload

	PRECACHE_SOUND("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND("weapons/scock1.wav");	// cock gun
}

int CM3::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		return TRUE;
	}
	return FALSE;
}
