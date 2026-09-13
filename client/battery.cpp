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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "utils.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Battery, Battery )
DECLARE_MESSAGE( m_Battery, Helmet )

static bool g_bBatteryHasHelmet = false;

int CHudBattery::Init( void )
{
	m_iBat = 0;
	g_bBatteryHasHelmet = false;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE( Battery );
	HOOK_MESSAGE( Helmet );

	gHUD.AddHudElem( this );

	return 1;
}

int CHudBattery::VidInit( void )
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( g_bBatteryHasHelmet ? "suithelmet_empty" : "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( g_bBatteryHasHelmet ? "suithelmet_full" : "suit_full" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = m_prc2 = NULL;
	m_iHeight = 0;
	if( HUD_suit_empty < 0 || HUD_suit_full < 0 )
		return 1;
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;

	return 1;
}

int CHudBattery:: MsgFunc_Battery(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	m_iFlags |= HUD_ACTIVE;
	int x = READ_SHORT();

	if( x != m_iBat )
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	END_READ();

	return 1;
}

int CHudBattery::MsgFunc_Helmet(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pszName, pbuf, iSize );
	bool hasHelmet = READ_BYTE() != 0;
	END_READ();

	if (hasHelmet == g_bBatteryHasHelmet)
		return 1;

	int emptyIndex = gHUD.GetSpriteIndex( hasHelmet ? "suithelmet_empty" : "suit_empty" );
	int fullIndex = gHUD.GetSpriteIndex( hasHelmet ? "suithelmet_full" : "suit_full" );
	if (emptyIndex < 0 || fullIndex < 0)
		return 1;

	g_bBatteryHasHelmet = hasHelmet;
	m_hSprite1 = gHUD.GetSprite( emptyIndex );
	m_hSprite2 = gHUD.GetSprite( fullIndex );
	m_prc1 = &gHUD.GetSpriteRect( emptyIndex );
	m_prc2 = &gHUD.GetSpriteRect( fullIndex );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = FADE_TIME;
	return 1;
}

int CHudBattery::Draw( float flTime )
{
	// Armor is drawn around the health silhouette in CHudHealth.
	return 1;
}
