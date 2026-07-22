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

#include "ammo_mp5clip.h"
#include "weapons/mp5.h"
#include "player.h"

LINK_ENTITY_TO_CLASS( ammo_mp5clip, CMP5AmmoClip );
LINK_ENTITY_TO_CLASS( ammo_9mmAR, CMP5AmmoClip );

void CMP5AmmoClip::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
	if (pev->health <= 0)
		pev->health = MP5_MAX_CLIP;
	CBasePlayerAmmo::Spawn( );
}
void CMP5AmmoClip::Precache()
{
	PRECACHE_MODEL ("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL CMP5AmmoClip::AddAmmo( CBaseEntity *pOther ) 
{ 
	CBasePlayer *player = static_cast<CBasePlayer *>(pOther);
	const int ammoType = player->GetAmmoIndex("9mm_mp5");
	const int rounds = Q_max(0, Q_min((int)pev->health, MP5_MAX_CLIP));
	int remaining = rounds;
	int bResult = player->AddMagazine(WEAPON_MP5, ammoType, rounds, MP5_MAX_CLIP, &remaining) > 0;
	if (bResult)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	}
	pev->health = remaining == 0 ? MP5_MAX_CLIP : remaining;
	return bResult && remaining == 0;
}
