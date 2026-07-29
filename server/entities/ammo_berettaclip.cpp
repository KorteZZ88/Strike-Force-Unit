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

#include "ammo_berettaclip.h"
#include "weapons/beretta.h"
#include "player.h"

LINK_ENTITY_TO_CLASS( ammo_berettaclip, CBerettaAmmo );

void CBerettaAmmo::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	if (pev->health <= 0)
		pev->health = BERETTA_MAX_CLIP;
	CBasePlayerAmmo::Spawn( );
}

void CBerettaAmmo::Precache()
{
	PRECACHE_MODEL ("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL CBerettaAmmo::AddAmmo( CBaseEntity *pOther ) 
{ 
	CBasePlayer *player = static_cast<CBasePlayer *>(pOther);
	const int ammoType = player->GetAmmoIndex("9mm_beretta");
	const int rounds = Q_max(0, Q_min((int)pev->health, BERETTA_MAX_CLIP));
	int remaining = rounds;
	if (player->AddMagazine(WEAPON_BERETTA, ammoType, rounds, BERETTA_MAX_CLIP, &remaining) > 0)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		pev->health = remaining == 0 ? BERETTA_MAX_CLIP : remaining;
		return remaining == 0;
	}
	return FALSE;
}
