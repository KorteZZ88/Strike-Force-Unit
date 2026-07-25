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
//
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "utils.h"
#include "parsemsg.h"
#include "ammohistory.h"
#include "weapons/glock.h"
#include "weapons/m24.h"
#include "weapons/usp.h"
#include "weapons/python.h"

int		g_weaponselect = 0;
WEAPON		*gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
WEAPON		*gpLastSel;	// Last weapon menu selection 
static wrect_t	nullRc;
WeaponsResource	gWR;

int WeaponsResource :: HasAmmo( WEAPON *p )
{
	if( !p )
		return FALSE;
	if( p->iMax1 == -1 )
		return TRUE;
	return (p->iAmmoType == -1) || p->iClip > 0 || CountAmmo( p->iAmmoType ) ||
		CountAmmo( p->iAmmo2Type ) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}

static bool HideEmptyBombGrenade(WEAPON *weapon)
{
	if (!weapon || gHUD.m_Teamplay != 2 || gWR.HasAmmo(weapon))
		return false;
	return !strcmp(weapon->szName, "weapon_handgrenade") ||
		!strcmp(weapon->szName, "weapon_flashbang") ||
		!strcmp(weapon->szName, "weapon_gasgrenade");
}

void WeaponsResource :: LoadWeaponSprites( WEAPON *pWeapon )
{
	int i, iRes;
	
	if( ScreenWidth < 640 )
		iRes = 320;
	else iRes = 640;

	char sz[128];

	if ( !pWeapon ) return;

	memset( &pWeapon->rcActive, 0, sizeof( wrect_t ));
	memset( &pWeapon->rcInactive, 0, sizeof( wrect_t ));
	memset( &pWeapon->rcAmmo, 0, sizeof( wrect_t ));
	memset( &pWeapon->rcAmmo2, 0, sizeof( wrect_t ));
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;
	
	// The construction wrench deliberately reuses the crowbar icon set.
	// Keep its own weapon classname while resolving only its HUD sprites through
	// weapon_crowbar.txt, which also works with the base-game mounted resources.
	const char *spriteWeaponName = !strcmp( pWeapon->szName, "weapon_wrench" )
		? "weapon_crowbar" : pWeapon->szName;
	// The timed satchel intentionally reuses the original satchel HUD artwork.
	if (!Q_stricmp(spriteWeaponName, "weapon_c4"))
		spriteWeaponName = "weapon_satchel";
	Q_snprintf( sz, sizeof( sz ), "sprites/%s.txt", spriteWeaponName );
	client_sprite_t *pList = SPR_GetList( sz, &i );

	if( !pList ) return;

	client_sprite_t *p;
	
	p = GetSpriteList( pList, "crosshair", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hCrosshair = SPR_Load( sz );
		pWeapon->rcCrosshair = p->rc;
	}
	else pWeapon->hCrosshair = 0;

	p = GetSpriteList( pList, "autoaim", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hAutoaim = SPR_Load( sz );
		pWeapon->rcAutoaim = p->rc;
	}
	else pWeapon->hAutoaim = 0;

	p = GetSpriteList( pList, "zoom", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hZoomedCrosshair = SPR_Load( sz );
		pWeapon->rcZoomedCrosshair = p->rc;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; // default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	p = GetSpriteList( pList, "zoom_autoaim", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hZoomedAutoaim = SPR_Load( sz );
		pWeapon->rcZoomedAutoaim = p->rc;
	}
	else
	{
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair;  // default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	p = GetSpriteList( pList, "weapon", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hInactive = SPR_Load( sz );
		pWeapon->rcInactive = p->rc;
		gHR.iHistoryGap = Q_max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
	{
		pWeapon->hInactive = gHUD.m_hHudError;
		pWeapon->rcInactive = gHUD.GetSpriteRect( gHUD.m_HUD_error );
		gHR.iHistoryGap = Q_max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}

	p = GetSpriteList( pList, "weapon_s", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hActive = SPR_Load( sz );
		pWeapon->rcActive = p->rc;
	}
	else
	{
		pWeapon->hActive = gHUD.m_hHudError;
		pWeapon->rcActive = gHUD.GetSpriteRect( gHUD.m_HUD_error );
	}

	p = GetSpriteList( pList, "ammo", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hAmmo = SPR_Load( sz );
		pWeapon->rcAmmo = p->rc;
		gHR.iHistoryGap = Q_max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else pWeapon->hAmmo = 0;

	p = GetSpriteList( pList, "ammo2", iRes, i );
	if( p )
	{
		Q_snprintf( sz, sizeof( sz ), "sprites/%s.spr", p->szSprite );
		pWeapon->hAmmo2 = SPR_Load( sz );
		pWeapon->rcAmmo2 = p->rc;
		gHR.iHistoryGap = Q_max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else pWeapon->hAmmo2 = 0;
}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	// Visit every configured HUD position, including the AK-47 primary slot.
	for( int i = 0; i < MAX_WEAPON_POSITIONS; i++ )
	{
		if( rgSlots[iSlot][i] && !HideEmptyBombGrenade(rgSlots[iSlot][i]) )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}
	return pret;
}

WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if( iSlotPos + 1 >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[iSlot][iSlotPos+1];
	
	if( !p || HideEmptyBombGrenade(p) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}

int	giBucketHeight;		// Ammo Bar width and height
int	giBucketWidth;
int	giABHeight;
int	giABWidth;

SpriteHandle	ghsprBuckets;		// Sprite for top row of weapons menu

DECLARE_MESSAGE( m_Ammo, CurWeapon  );	// Current weapon and clip
DECLARE_MESSAGE( m_Ammo, WeaponList );	// new weapon type
DECLARE_MESSAGE( m_Ammo, AmmoX );	// update known ammo type's count
DECLARE_MESSAGE( m_Ammo, Magazines );
DECLARE_MESSAGE( m_Ammo, PickupHint );
DECLARE_MESSAGE( m_Ammo, BaseStatus );
DECLARE_MESSAGE( m_Ammo, AmmoPickup );	// flashes an ammo pickup record
DECLARE_MESSAGE( m_Ammo, WeapPickup );	// flashes a weapon pickup record
DECLARE_MESSAGE( m_Ammo, HideWeapon );	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE( m_Ammo, ItemPickup );

DECLARE_COMMAND( m_Ammo, Slot1 );
DECLARE_COMMAND( m_Ammo, Slot2 );
DECLARE_COMMAND( m_Ammo, Slot3 );
DECLARE_COMMAND( m_Ammo, Slot4 );
DECLARE_COMMAND( m_Ammo, Slot5 );
DECLARE_COMMAND( m_Ammo, Slot6 );
DECLARE_COMMAND( m_Ammo, Slot7 );
DECLARE_COMMAND( m_Ammo, Slot8 );
DECLARE_COMMAND( m_Ammo, Slot9 );
DECLARE_COMMAND( m_Ammo, Slot10 );
DECLARE_COMMAND( m_Ammo, Close );
DECLARE_COMMAND( m_Ammo, NextWeapon );
DECLARE_COMMAND( m_Ammo, PrevWeapon );

// width of ammo fonts
#define AMMO_SMALL_WIDTH	10
#define AMMO_LARGE_WIDTH	20
#define HISTORY_DRAW_TIME	"5"

int CHudAmmo::Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( CurWeapon );
	HOOK_MESSAGE( WeaponList );
	HOOK_MESSAGE( AmmoPickup );
	HOOK_MESSAGE( WeapPickup );
	HOOK_MESSAGE( ItemPickup );
	HOOK_MESSAGE( HideWeapon );
	HOOK_MESSAGE( AmmoX );
	HOOK_MESSAGE( Magazines );
	HOOK_MESSAGE( PickupHint );
	HOOK_MESSAGE( BaseStatus );

	HOOK_COMMAND( "slot1", Slot1 );
	HOOK_COMMAND( "slot2", Slot2 );
	HOOK_COMMAND( "slot3", Slot3 );
	HOOK_COMMAND( "slot4", Slot4 );
	HOOK_COMMAND( "slot5", Slot5 );
	HOOK_COMMAND( "slot6", Slot6 );
	HOOK_COMMAND( "slot7", Slot7 );
	HOOK_COMMAND( "slot8", Slot8 );
	HOOK_COMMAND( "slot9", Slot9 );
	HOOK_COMMAND( "slot10", Slot10 );
	HOOK_COMMAND( "cancelselect", Close );
	HOOK_COMMAND( "invnext", NextWeapon );
	HOOK_COMMAND( "invprev", PrevWeapon );

	Reset();

	CVAR_REGISTER( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );

	// controls whether or not weapons can be selected in one keypress
	CVAR_REGISTER( "hud_fastswitch", "0", FCVAR_ARCHIVE );

	// Counter-Strike style automatic switch to a more valuable weapon on pickup.
	// USERINFO mirrors the preference to the server, which owns weapon selection.
	CVAR_REGISTER( "cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
	CVAR_REGISTER( "cl_autoreload", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );

	m_iFlags |= HUD_ACTIVE; //!!!

	gWR.Init();
	gHR.Init();

	return 1;
}

void CHudAmmo::Reset( void )
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();

	SetCrosshair( 0, nullRc, 0, 0, 0 );	// reset crosshair
	m_pWeapon = NULL;			// reset last weapon
	m_iMagazineType = 0;
	m_bMergingMagazines = false;
	m_szPickupHint[0] = '\0';
	m_flPickupHintUntil = 0.0f;
	m_bBaseStatusVisible = false;
	m_iBaseHealth = m_iBaseAmmoPoints = m_iBaseBuildPoints = 0;
	m_iBaseMaxHealth = 100;
	m_bReloadHoldActive = false;
	m_bReloadHoldCompleted = false;
	m_flReloadHoldStart = 0.0f;
	memset(m_rgMagazineRounds, 0, sizeof(m_rgMagazineRounds));
	memset(m_rgMagazineCapacities, 0, sizeof(m_rgMagazineCapacities));
}

int CHudAmmo::VidInit( void )
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );
	const int magazineEmpty = gHUD.GetSpriteIndex("flash_empty");
	const int magazineFull = gHUD.GetSpriteIndex("flash_full");
	m_hMagazineEmpty = gHUD.GetSprite(magazineEmpty);
	m_hMagazineFull = gHUD.GetSprite(magazineFull);
	m_rcMagazineEmpty = gHUD.GetSpriteRect(magazineEmpty);
	m_rcMagazineFull = gHUD.GetSpriteRect(magazineFull);

	ghsprBuckets = gHUD.GetSprite( m_HUD_bucket0 );
	giBucketWidth = gHUD.GetSpriteRect( m_HUD_bucket0 ).right - gHUD.GetSpriteRect( m_HUD_bucket0 ).left;
	giBucketHeight = gHUD.GetSpriteRect( m_HUD_bucket0 ).bottom - gHUD.GetSpriteRect( m_HUD_bucket0 ).top;

	gHR.iHistoryGap = Q_max( gHR.iHistoryGap, gHUD.GetSpriteRect( m_HUD_bucket0 ).bottom - gHUD.GetSpriteRect( m_HUD_bucket0 ).top );

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	if( ScreenWidth >= 640 )
	{
		giABWidth = 20;
		giABHeight = 4;
	}
	else
	{
		giABWidth = 10;
		giABHeight = 2;
	}

	return 1;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think( void )
{
	if( gHUD.m_fPlayerDead )
		return;

	if( memcmp( gHUD.m_iWeaponBits, gWR.iOldWeaponBits, MAX_WEAPON_BYTES ))
	{
		memcpy( gWR.iOldWeaponBits, gHUD.m_iWeaponBits, MAX_WEAPON_BYTES );

		for( int i = MAX_WEAPONS - 1; i > 0; i-- )
		{
			WEAPON *p = gWR.GetWeapon( i );

			if( p )
			{
				if( gHUD.HasWeapon( p->iId ))
					gWR.PickupWeapon( p );
				else
					gWR.DropWeapon( p );
			}
		}
	}

	if( !gpActiveSel )
		return;

	// has the player selected one?
	if( gHUD.m_iKeyBits & IN_ATTACK )
	{
		if( gpActiveSel != (WEAPON *)1 )
		{
			ServerCmd( gpActiveSel->szName );
			g_weaponselect = gpActiveSel->iId;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound( "common/wpn_select.wav", 1 );
	}

}

//
// Helper function to return a Ammo pointer from id
//
SpriteHandle* WeaponsResource :: GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect )
{
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if( rgWeapons[i].iAmmoType == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if( rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}
	return NULL;
}


// Menu Selection Code
void WeaponsResource :: SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if( gHUD.m_Menu.m_fMenuDisplayed && ( fAdvance == FALSE ) && ( iDirection == 1 ))	
	{
		// menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if( iSlot > MAX_WEAPON_SLOTS )
		return;

	if((gHUD.m_fPlayerDead&&!gHUD.m_bInEyeSpectator)||gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ))
		return;

	if( !gHUD.HasWeapon( WEAPON_SUIT ))
		return;

	if ( !memcmp( gHUD.m_iWeaponBits, nullbits, sizeof( gHUD.m_iWeaponBits )))
		return;

	WEAPON *p = NULL;
	bool fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" ) != 0;

	if(( gpActiveSel == NULL ) || ( gpActiveSel == (WEAPON *)1 ) || ( iSlot != gpActiveSel->iSlot ))
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		p = GetFirstPos( iSlot );

		if( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );

			if( !p2 )
			{	
				// only one active item in bucket, so change directly to weapon
				ServerCmd( p->szName );
				g_weaponselect = p->iId;
				return;
			}
		}
	}
	else
	{
		PlaySound( "common/wpn_moveselect.wav", 1 );
		if( gpActiveSel )
			p = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if( !p )
			p = GetFirstPos( iSlot );
	}

	
	if( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if( !fastSwitch )
			gpActiveSel = (WEAPON *)1;
		else
			gpActiveSel = NULL;
	}
	else 
		gpActiveSel = p;
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	gWR.SetAmmo( iIndex, abs( iCount ));

	END_READ();

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	// Add ammo to the history
	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs( iCount ));

	END_READ();

	return 1;
}

int CHudAmmo::MsgFunc_Magazines(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pszName, pbuf, iSize);
	m_iMagazineType = READ_BYTE();
	for (int slot = 0; slot < 6; ++slot)
	{
		m_rgMagazineRounds[slot] = READ_BYTE();
		m_rgMagazineCapacities[slot] = READ_BYTE();
	}
	m_bMergingMagazines = READ_BYTE() != 0;
	END_READ();
	return 1;
}

int CHudAmmo::MsgFunc_PickupHint(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pszName, pbuf, iSize);
	Q_strncpy(m_szPickupHint, READ_STRING(), sizeof(m_szPickupHint));
	m_flPickupHintUntil = m_szPickupHint[0] ? gHUD.m_flTime + 1.5f : 0.0f;
	END_READ();
	return 1;
}

int CHudAmmo::MsgFunc_BaseStatus(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pszName, pbuf, iSize);
	m_bBaseStatusVisible = READ_BYTE() != 0;
	m_iBaseHealth = READ_SHORT();
	m_iBaseAmmoPoints = READ_SHORT();
	m_iBaseBuildPoints = READ_SHORT();
	m_iBaseMaxHealth = READ_SHORT();
	END_READ();
	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );
	int iIndex = READ_BYTE();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );

	END_READ();

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_ITEM, szName );

	END_READ();

	return 1;
}

int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if(( m_pWeapon == NULL ) || ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )))
	{
		gpActiveSel = NULL;
		SetCrosshair( 0, nullRc, 0, 0, 0 );
	}
	else
	{
		SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255 );
	}

	END_READ();

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	int fOnTarget = FALSE;

	BEGIN_READ( pszName, pbuf, iSize );

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	// detect if we're also on target
	if( iState > 1 )
	{
		fOnTarget = TRUE;
	}

	if( iId < 1 )
	{
		SetCrosshair( 0, nullRc, 0, 0, 0 );
		m_pWeapon = NULL;
		return 0;
	}

	// Is player dead???
	if(( iId == -1 ) && ( iClip == -1 ))
	{
		gHUD.m_fPlayerDead = TRUE;
		gpActiveSel = NULL;
		return 1;
	}

	gHUD.m_fPlayerDead = FALSE;

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if( !pWeapon )
		return 0;

	if( iClip < -1 )
		pWeapon->iClip = abs( iClip );
	else
		pWeapon->iClip = iClip;


	if( iState == 0 )	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	if( gHUD.m_iFOV >= 90 )
	{ 
		// normal crosshairs
		if( fOnTarget && m_pWeapon->hAutoaim )
			SetCrosshair( m_pWeapon->hAutoaim, m_pWeapon->rcAutoaim, 255, 255, 255 );
		else SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255 );
	}
	else
	{	// zoomed crosshairs
		if( fOnTarget && m_pWeapon->hZoomedAutoaim )
			SetCrosshair( m_pWeapon->hZoomedAutoaim, m_pWeapon->rcZoomedAutoaim, 255, 255, 255 );
		else SetCrosshair( m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, 255, 255, 255 );

	}

	m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;

	END_READ();
	
	return 1;
}

//
// WeaponList -- Tells the hud about a new weapon type.
//
int CHudAmmo::MsgFunc_WeaponList( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );
	
	WEAPON Weapon;

	Q_strcpy( Weapon.szName, READ_STRING() );
	Weapon.iAmmoType = (int)READ_CHAR();	
	
	Weapon.iMax1 = READ_BYTE();
	if( Weapon.iMax1 == 255 )
		Weapon.iMax1 = -1;

	Weapon.iAmmo2Type = READ_CHAR();
	Weapon.iMax2 = READ_BYTE();
	if( Weapon.iMax2 == 255 )
		Weapon.iMax2 = -1;

	Weapon.iSlot = READ_CHAR();
	Weapon.iSlotPos = READ_CHAR();
	Weapon.iId = READ_CHAR();
	Weapon.iFlags = READ_BYTE();
	Weapon.iClip = 0;

	gWR.AddWeapon( &Weapon );
	// WeaponList and Weapons are independent messages.  If this description is
	// the later one, complete the same ownership synchronization here.
	if( gHUD.HasWeapon( Weapon.iId ))
		gWR.PickupWeapon( gWR.GetWeapon( Weapon.iId ));

	END_READ();

	return 1;

}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
	gWR.SelectSlot( iSlot, FALSE, 1 );
}

void CHudAmmo::UserCmd_Slot1( void )
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2( void )
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3( void )
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4( void )
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5( void )
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6( void )
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7( void )
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8( void )
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9( void )
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10( void )
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Close( void )
{
	if( gpActiveSel )
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound( "common/wpn_hudoff.wav", 1 );
	}
	else
		ClientCmd( "escape" ); // go into menu
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon( void )
{
	if((gHUD.m_fPlayerDead&&!gHUD.m_bInEyeSpectator)||(gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )))
		return;

	if( !gpActiveSel || gpActiveSel == (WEAPON *)1 )
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;

	if( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos + 1;
		slot = gpActiveSel->iSlot;
	}

	for( int loop = 0; loop <= 1; loop++ )
	{
		for( ; slot < MAX_WEAPON_SLOTS; slot++ )
		{
			for( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if( wsp && !HideEmptyBombGrenade(wsp) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon( void )
{
	if((gHUD.m_fPlayerDead&&!gHUD.m_bInEyeSpectator)||(gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )))
		return;

	if( !gpActiveSel || gpActiveSel == (WEAPON *)1 )
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS - 1;
	int slot = MAX_WEAPON_SLOTS - 1;

	if( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos - 1;
		slot = gpActiveSel->iSlot;
	}
	
	for( int loop = 0; loop <= 1; loop++ )
	{
		for( ; slot >= 0; slot-- )
		{
			for( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if( wsp && !HideEmptyBombGrenade(wsp) )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS - 1;
		}
		
		slot = MAX_WEAPON_SLOTS - 1;
	}

	gpActiveSel = NULL;
}

//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------
int CHudAmmo::Draw( float flTime )
{
	if (m_szPickupHint[0] && m_flPickupHintUntil > 0.0f && flTime >= m_flPickupHintUntil)
		m_szPickupHint[0] = '\0';
	int a, x, y, r, g, b;
	int AmmoWidth;

	if (!gHUD.HasWeapon( WEAPON_SUIT ))
		return 1;

	if(( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )))
		return 1;

	if( m_bBaseStatusVisible )
	{
		const int barX = 16, barY = 16, barWidth = Q_max( 120, ScreenWidth / 6 ), barHeight = 9;
		const float fraction = m_iBaseMaxHealth > 0 ? bound( 0.0f, (float)m_iBaseHealth / m_iBaseMaxHealth, 1.0f ) : 0.0f;
		char ammoPoints[32], buildPoints[32];
		Q_snprintf( ammoPoints, sizeof( ammoPoints ), "Ammo: %d", m_iBaseAmmoPoints );
		Q_snprintf( buildPoints, sizeof( buildPoints ), "Build: %d", m_iBaseBuildPoints );
		int ammoWidth = 0;
		for( const byte *ch = reinterpret_cast<const byte *>( ammoPoints ); *ch; ++ch )
			ammoWidth += gHUD.m_scrinfo.charWidths[*ch];
		// FillRGBABlend keeps the health bar itself opaque without requiring
		// a background panel behind the resource labels.
		gEngfuncs.pfnFillRGBABlend( barX - 1, barY - 1, barWidth + 2, barHeight + 2, 0, 0, 0, 255 );
		gEngfuncs.pfnFillRGBABlend( barX, barY, barWidth, barHeight, 20, 35, 55, 255 );
		int healthR = 40, healthG = 120, healthB = 255;
		if( fraction <= 0.30f )
		{
			healthR = 235; healthG = 45; healthB = 35;
		}
		else if( fraction <= 0.75f )
		{
			healthR = 245; healthG = 205; healthB = 35;
		}
		gEngfuncs.pfnFillRGBABlend( barX, barY, (int)( barWidth * fraction ), barHeight,
			healthR, healthG, healthB, 255 );
		gHUD.DrawHudString( barX, barY + 14, ScreenWidth, ammoPoints, 255, 55, 55 );
		gHUD.DrawHudString( barX + ammoWidth + 12, barY + 14, ScreenWidth, buildPoints, 255, 150, 30 );
	}

	// Draw Weapon Menu
	DrawWList( flTime );

	// Draw ammo pickup history
	gHR.DrawAmmoHistory( flTime );

	if (m_bMergingMagazines || m_szPickupHint[0])
	{
		char mergeMessage[] = "Merging magazines...";
		char *message = m_bMergingMagazines ? mergeMessage : m_szPickupHint;
		int messageWidth = 0;
		for (const byte *ch = reinterpret_cast<const byte *>(message); *ch; ++ch)
			messageWidth += gHUD.m_scrinfo.charWidths[*ch];
		const int messageX = Q_max(0, (ScreenWidth - messageWidth) / 2);
		const int messageY = ScreenHeight * 3 / 4;
		gHUD.DrawHudString(messageX, messageY, ScreenWidth, message,
			gHUD.m_color.r, gHUD.m_color.g, gHUD.m_color.b);
	}

	int fullestSpareMagazine = 0;
	for (int slot = 0; slot < 6; ++slot)
	{
		if (m_rgMagazineCapacities[slot] > 0)
			fullestSpareMagazine = Q_max(fullestSpareMagazine, m_rgMagazineRounds[slot]);
	}
	const int currentClip = m_pWeapon ? m_pWeapon->iClip : 0;
	const int reloadResult = fullestSpareMagazine + (currentClip > 0 ? 1 : 0);
	const bool canReloadMagazine = m_pWeapon && m_pWeapon->iId == m_iMagazineType &&
		fullestSpareMagazine > 0 && reloadResult != currentClip;
	const bool reloadHeld = (gHUD.m_iKeyBits & IN_RELOAD) != 0 &&
		canReloadMagazine && !m_bMergingMagazines;
	if (!reloadHeld)
	{
		m_bReloadHoldActive = false;
		m_bReloadHoldCompleted = false;
	}
	else if (!m_bReloadHoldCompleted)
	{
		if (!m_bReloadHoldActive)
		{
			m_bReloadHoldActive = true;
			m_flReloadHoldStart = flTime;
		}

		const float progress = bound(0.0f, (flTime - m_flReloadHoldStart) / 0.5f, 1.0f);
		const int barWidth = bound(96, ScreenWidth / 5, 180);
		const int barHeight = 6;
		const int barX = (ScreenWidth - barWidth) / 2;
		const int barY = ScreenHeight * 13 / 20;
		FillRGBA(barX - 1, barY - 1, barWidth + 2, barHeight + 2, 0, 0, 0, 180);
		FillRGBA(barX, barY, barWidth, barHeight, 32, 32, 32, 180);
		if (progress > 0.0f)
		{
			FillRGBA(barX, barY, Q_max(1, (int)(barWidth * progress)), barHeight,
				gHUD.m_color.r, gHUD.m_color.g, gHUD.m_color.b, 220);
		}
		if (progress >= 1.0f)
			m_bReloadHoldCompleted = true;
	}

	if( !( m_iFlags & HUD_ACTIVE ))
		return 0;

	if( !m_pWeapon )
	{
		return 0;
	}

	WEAPON *pw = m_pWeapon; // shorthand

	// SPR_Draw Ammo
	if(( pw->iAmmoType < 0 ) && ( pw->iAmmo2Type < 0 ))
		return 0;


	int iFlags = DHN_DRAWZERO; // draw 0 values

	AmmoWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;

	a = (int)Q_max( MIN_ALPHA, m_fFade );

	if( m_fFade > 0 )
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	r = gHUD.m_color.r;
	g = gHUD.m_color.g;
	b = gHUD.m_color.b;

	ScaleColors( r, g, b, a );

	// Does this weapon have a clip?
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;

	// Does weapon have any ammo at all?
	if( m_pWeapon->iAmmoType > 0 )
	{
		int iIconWidth = m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left;
		
		if( pw->iClip >= 0 )
		{
			if (m_iMagazineType == pw->iId)
			{
				const int spriteWidth = m_rcMagazineEmpty.right - m_rcMagazineEmpty.left;
				const int spriteHeight = m_rcMagazineEmpty.bottom - m_rcMagazineEmpty.top;
				const int magazineX = ScreenWidth - spriteWidth - spriteWidth / 2;
				int magazineSlots = 6;
				if (m_iMagazineType == WEAPON_BERETTA)
					magazineSlots = BERETTA_MAX_SPARE_MAGAZINES;
				else if (m_iMagazineType == WEAPON_USP)
					magazineSlots = USP_MAX_SPARE_MAGAZINES;
				else if (m_iMagazineType == WEAPON_M24)
					magazineSlots = M24_MAX_SPARE_MAGAZINES;
				else if (m_iMagazineType == WEAPON_PYTHON)
					magazineSlots = PYTHON_MAX_SPARE_MAGAZINES;
				for (int slot = 0; slot < magazineSlots; ++slot)
				{
					const int magazineY = y - slot * (spriteHeight + 1);
					const int capacity = m_rgMagazineCapacities[slot];
					const int rounds = m_rgMagazineRounds[slot];
					const bool hasMagazine = capacity > 0 && rounds > 0;
					int mr = 48, mg = 48, mb = 48;
					float magazineFill = 0.0f;
					if (hasMagazine)
					{
						magazineFill = bound(0.0f, (float)rounds / (float)capacity, 1.0f);
						if (magazineFill < 0.2f)
						{
							UnpackRGB(mr, mg, mb, RGB_REDISH);
						}
						else
						{
							mr = gHUD.m_color.r;
							mg = gHUD.m_color.g;
							mb = gHUD.m_color.b;
						}
					}
					ScaleColors(mr, mg, mb, hasMagazine ? a : MIN_ALPHA);
					SPR_Set(m_hMagazineEmpty, mr, mg, mb);
					SPR_DrawAdditive(0, magazineX, magazineY, &m_rcMagazineEmpty);
					if (magazineFill > 0.0f)
					{
						wrect_t fill = m_rcMagazineFull;
						const int fullWidth = fill.right - fill.left;
						const int offset = (int)(fullWidth * (1.0f - magazineFill));
						fill.left += offset;
						SPR_Set(m_hMagazineFull, mr, mg, mb);
						SPR_DrawAdditive(0, magazineX + offset, magazineY, &fill);
					}
				}
				r = gHUD.m_color.r;
				g = gHUD.m_color.g;
				b = gHUD.m_color.b;
				ScaleColors(r, g, b, a);
				x = magazineX - 4 * AmmoWidth;
				x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, pw->iClip, r, g, b);
			}
			else
			{
				// room for the number and the '|' and the current ammo
				x = ScreenWidth - ( 8 * AmmoWidth ) - iIconWidth;
				x = gHUD.DrawHudNumber( x, y, iFlags | DHN_3DIGITS, pw->iClip, r, g, b );
				int iBarWidth = AmmoWidth / 10;
				x += AmmoWidth / 2;
				r = gHUD.m_color.r;
				g = gHUD.m_color.g;
				b = gHUD.m_color.b;
				FillRGBA( x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a );
				x += iBarWidth + AmmoWidth / 2;
				ScaleColors( r, g, b, a );
				x = gHUD.DrawHudNumber( x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo( pw->iAmmoType ), r, g, b );
			}
		}
		else
		{
			// SPR_Draw a bullets only line
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo( pw->iAmmoType ), r, g, b );
		}

		if (m_iMagazineType != pw->iId)
		{
			// Draw the ammo Icon
			int iOffset = ( m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top ) / 8;
			SPR_Set( m_pWeapon->hAmmo, r, g, b );
			SPR_DrawAdditive( 0, x, y - iOffset, &m_pWeapon->rcAmmo );
		}
	}

	// Does weapon have seconday ammo?
	if( pw->iAmmo2Type > 0 ) 
	{
		int iIconWidth = m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left;

		// Do we have secondary ammo?
		if(( pw->iAmmo2Type != 0 ) && ( gWR.CountAmmo(pw->iAmmo2Type ) > 0))
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight / 4;
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber( x, y, iFlags|DHN_3DIGITS, gWR.CountAmmo( pw->iAmmo2Type ), r, g, b );

			// Draw the ammo Icon
			SPR_Set( m_pWeapon->hAmmo2, r, g, b );
			int iOffset = ( m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top ) / 8;
			SPR_DrawAdditive( 0, x, y - iOffset, &m_pWeapon->rcAmmo2 );
		}
	}
	return 1;
}

#include <mathlib.h>

//
// Draws the ammo bar on the hud
//
int DrawBar( int x, int y, int width, int height, float f )
{
	int r, g, b;

	f = bound( 0.0f, f, 1.0f );

	if( f )
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if( w <= 0 ) w = 1;

		UnpackRGB( r, g, b, RGB_GREENISH );
		FillRGBA( x, y, w, height, r, g, b, 255 );
		x += w;
		width -= w;
	}

	r = gHUD.m_color.r;
	g = gHUD.m_color.g;
	b = gHUD.m_color.b;

	FillRGBA( x, y, width, height, r, g, b, 128 );

	return (x + width);
}

void DrawAmmoBar( WEAPON *p, int x, int y, int width, int height )
{
	if( !p )
		return;
	
	if( p->iAmmoType != -1 )
	{
		if( !gWR.CountAmmo( p->iAmmoType ))
			return;

		float f = (float)gWR.CountAmmo(p->iAmmoType) / (float)p->iMax1;
		
		x = DrawBar( x, y, width, height, f );

		// Do we have secondary ammo too?
		if( p->iAmmo2Type != -1 )
		{
			f = (float)gWR.CountAmmo(p->iAmmo2Type) / (float)p->iMax2;

			x += 5; //!!!
			DrawBar( x, y, width, height, f );
		}
	}
}

//
// Draw Weapon Menu
//
int CHudAmmo::DrawWList( float flTime )
{
	int r, g, b, a;
	int x, y, i;

	if( !gpActiveSel )
		return 0;

	int iActiveSlot;

	if( gpActiveSel == (WEAPON *)1 )
		iActiveSlot = -1;	// current slot has no weapons
	else 
		iActiveSlot = gpActiveSel->iSlot;

	x = 10; //!!!
	y = 10; //!!!

	// Ensure that there are available choices in the active slot
	if( iActiveSlot > 0 )
	{
		if( !gWR.GetFirstPos( iActiveSlot ))
		{
			gpActiveSel = (WEAPON *)1;
			iActiveSlot = -1;
		}
	}
		
	// Draw top line
	for( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		int iWidth;

		r = gHUD.m_color.r;
		g = gHUD.m_color.g;
		b = gHUD.m_color.b;
	
		if( iActiveSlot == i )
			a = 255;
		else
			a = 192;

		ScaleColors( r, g, b, 255 );
		SPR_Set( gHUD.GetSprite( m_HUD_bucket0 + i ), r, g, b );

		// make active slot wide enough to accomodate gun pictures
		if( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos(iActiveSlot);

			if( p )
				iWidth = p->rcActive.right - p->rcActive.left;
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( m_HUD_bucket0 + i ));
		
		x += iWidth + 5;
	}

	a = 128; //!!!
	x = 10;

	// Draw all of the buckets
	for( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );
			int iWidth = giBucketWidth;

			if( p )
				iWidth = p->rcActive.right - p->rcActive.left;

			for( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if( !p || !p->iId || HideEmptyBombGrenade(p) )
					continue;

				r = gHUD.m_color.r;
				g = gHUD.m_color.g;
				b = gHUD.m_color.b;
			
				// if active, then we must have ammo.
				if( gpActiveSel == p )
				{
					if( !gWR.HasAmmo( p ))
						UnpackRGB( r, g, b, RGB_REDISH );
					SPR_Set( p->hActive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &p->rcActive );

					SPR_Set( gHUD.GetSprite( m_HUD_selection ), r, g, b );
					SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( m_HUD_selection ));
				}
				else
				{
					// Draw Weapon if Red if no ammo
					if( gWR.HasAmmo( p ))
					{
						ScaleColors( r, g, b, 192 );
					}
					else
					{
						UnpackRGB( r, g, b, RGB_REDISH );
						ScaleColors( r, g, b, 128 );
					}

					SPR_Set( p->hInactive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &p->rcInactive );
				}

				// Draw Ammo Bar
				DrawAmmoBar( p, x + giABWidth / 2, y, giABWidth, giABHeight );
				y += p->rcActive.bottom - p->rcActive.top + 5;
			}

			x += iWidth + 5;

		}
		else
		{
			// Draw Row of weapons.
			r = gHUD.m_color.r;
			g = gHUD.m_color.g;
			b = gHUD.m_color.b;

			for( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( i, iPos );
				
				if( !p || !p->iId || HideEmptyBombGrenade(p) )
					continue;

				if( gWR.HasAmmo( p ))
				{
					r = gHUD.m_color.r;
					g = gHUD.m_color.g;
					b = gHUD.m_color.b;
					a = 128;
				}
				else
				{
					UnpackRGB( r, g, b, RGB_REDISH );
					a = 96;
				}

				FillRGBA( x, y, giBucketWidth, giBucketHeight, r, g, b, a );
				y += giBucketHeight + 5;
			}
			x += giBucketWidth + 5;
		}
	}	

	return 1;

}
