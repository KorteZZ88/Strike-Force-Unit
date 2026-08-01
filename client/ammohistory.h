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

#ifndef AMMOHISTORY_H
#define AMMOHISTORY_H

// Expose every primary weapon position through AUG at position 15.
#define MAX_WEAPON_POSITIONS		16

class WeaponsResource
{
private:
	// Information about weapons & ammo
	WEAPON		rgWeapons[MAX_WEAPONS];	// Weapons Array

	// ugly hack to compare array
	byte		nullbits[MAX_WEAPON_BYTES];

	// counts of weapons * ammo

	// The slots currently in use by weapons.  The value is a pointer to the weapon;
	// if it's NULL, no weapon is there
	WEAPON		*rgSlots[MAX_WEAPON_SLOTS+1][MAX_WEAPON_POSITIONS+1];

	// count of each ammo type
	int		riAmmo[MAX_AMMO_SLOTS];

public:
	void Init( void )
	{
		memset( rgWeapons, 0, sizeof rgWeapons );
		memset( nullbits, 0, MAX_WEAPON_BYTES );
		SetBits( nullbits[WEAPON_SUIT >> 3], BIT( WEAPON_SUIT & 7 ));
		Reset();
	}

	void Reset( void )
	{
		memset( iOldWeaponBits, 0, MAX_WEAPON_BYTES );
		memset( rgSlots, 0, sizeof rgSlots );
		memset( riAmmo, 0, sizeof riAmmo );
	}

///// WEAPON /////
	byte	iOldWeaponBits[MAX_WEAPON_BYTES];

	WEAPON *GetWeapon( int iId ) { return iId >= 0 && iId < MAX_WEAPONS ? &rgWeapons[iId] : NULL; }
	void AddWeapon( WEAPON *wp ) 
	{ 
		if( !wp || wp->iId <= 0 || wp->iId >= MAX_WEAPONS ||
			wp->iSlot < 0 || wp->iSlot >= MAX_WEAPON_SLOTS ||
			wp->iSlotPos < 0 || wp->iSlotPos >= MAX_WEAPON_POSITIONS )
			return;
		rgWeapons[wp->iId] = *wp;	
		LoadWeaponSprites( &rgWeapons[ wp->iId ] );
	}

	void PickupWeapon( WEAPON *wp )
	{
		if( !wp || wp->iSlot < 0 || wp->iSlot >= MAX_WEAPON_SLOTS ||
			wp->iSlotPos < 0 || wp->iSlotPos >= MAX_WEAPON_POSITIONS )
			return;
		rgSlots[wp->iSlot][wp->iSlotPos] = wp;
	}

	void DropWeapon( WEAPON *wp )
	{
		if( !wp || wp->iSlot < 0 || wp->iSlot >= MAX_WEAPON_SLOTS ||
			wp->iSlotPos < 0 || wp->iSlotPos >= MAX_WEAPON_POSITIONS )
			return;
		// Team-specific weapons can deliberately share a HUD position. Never
		// let removal of one weapon erase another weapon currently in that cell.
		if( rgSlots[wp->iSlot][wp->iSlotPos] == wp )
			rgSlots[wp->iSlot][wp->iSlotPos] = NULL;
	}

	void DropAllWeapons( void )
	{
		for( int i = 0; i < MAX_WEAPONS; i++ )
		{
			if( rgWeapons[i].iId )
				DropWeapon( &rgWeapons[i] );
		}
	}

	void LoadAllWeaponSprites( void )
	{
		for( int i = 0; i < MAX_WEAPONS; i++ )
		{
			if( rgWeapons[i].iId )
				LoadWeaponSprites( &rgWeapons[i] );
		}
	}

	WEAPON	*GetWeaponSlot( int slot, int pos ) { return slot >= 0 && slot < MAX_WEAPON_SLOTS && pos >= 0 && pos < MAX_WEAPON_POSITIONS ? rgSlots[slot][pos] : NULL; }

	void	LoadWeaponSprites( WEAPON* wp );
	WEAPON	*GetFirstPos( int iSlot );
	void	SelectSlot( int iSlot, int fAdvance, int iDirection );
	WEAPON	*GetNextActivePos( int iSlot, int iSlotPos );

	int	HasAmmo( WEAPON *p );

///// AMMO /////
	int	GetAmmo( int iId ) { return iId; }
	void	SetAmmo( int iId, int iCount ) { riAmmo[ iId ] = iCount;	}
	SpriteHandle	*GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect );

	int CountAmmo( int iId ) 
	{
		if( iId < 0 ) return 0;
		return riAmmo[iId];
	}
};

extern WeaponsResource gWR;

#define MAX_HISTORY		12

enum
{
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
};

class HistoryResource
{
private:
	struct HIST_ITEM
	{
		int type;
		float DisplayTime;  // the time at which this item should be removed from the history
		int iCount;
		int iId;
	};

	HIST_ITEM	rgAmmoHistory[MAX_HISTORY];
public:
	void Init( void )
	{
		Reset();
	}

	void Reset( void )
	{
		memset( rgAmmoHistory, 0, sizeof rgAmmoHistory );
	}

	int iHistoryGap;
	int iCurrentHistorySlot;

	void AddToHistory( int iType, int iId, int iCount = 0 );
	void AddToHistory( int iType, const char *szName, int iCount = 0 );

	void CheckClearHistory( void );
	int DrawAmmoHistory( float flTime );
};

extern HistoryResource gHR;

#endif//AMMOHISTORY_H
