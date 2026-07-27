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

#include "weapon_python.h"
#include "weapon_layer.h"
#include "weapons/python.h"
#include "server_weapon_layer_impl.h"
#include "viewmodel_sounds.h"

LINK_ENTITY_TO_CLASS( weapon_python, CRBull );
LINK_ENTITY_TO_CLASS( weapon_357, CRBull );
LINK_ENTITY_TO_CLASS( weapon_rbull, CRBull );

CRBull::CRBull()
{
	auto layerImpl = std::make_unique<CServerWeaponLayerImpl>(this);
	auto contextImpl = std::make_unique<CRBullWeaponContext>(std::move(layerImpl));
	m_pWeaponContext = std::move(contextImpl);
}

void CRBull::Spawn()
{
	pev->classname = MAKE_STRING(CLASSNAME_STR(RBULL_CLASSNAME)); // Keep old entity names as aliases.
	Precache();
	SET_MODEL(ENT(pev), "models/weapon/RBull/w_rbull.mdl");
	FallInit(); // get ready to fall down.
}

void CRBull::Precache()
{
	PRECACHE_MODEL("models/weapon/RBull/v_rbull.mdl");
	PRECACHE_MODEL("models/weapon/RBull/w_rbull.mdl");
	PRECACHE_MODEL("models/weapon/RBull/p_rbull.mdl");
	PrecacheViewModelSounds("models/weapon/RBull/v_rbull.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/357_reload1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("weapons/357_shot1.wav");
	PRECACHE_SOUND("weapons/357_shot2.wav");

	PRECACHE_MODEL("models/shell.mdl"); // brass shell
}

int CRBull::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		return TRUE;
	}
	return FALSE;
}
