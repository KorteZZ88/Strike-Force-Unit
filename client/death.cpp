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
// death notice
//
#include "hud.h"
#include "utils.h"
#include "parsemsg.h"

struct DeathNoticeItem
{
	char	szKiller[MAX_PLAYER_NAME_LENGTH*2];
	char	szVictim[MAX_PLAYER_NAME_LENGTH*2];
	int	iId; // the index number of the associated sprite
	int	iSuicide;
	int	iTeamKill;
	int	iNonPlayerKill;
	float	flDisplayTime;
	float	*KillerColor;
	float	*VictimColor;
};

#define MAX_DEATHNOTICES		4
#define DEATHNOTICE_TOP		32

static int DEATHNOTICE_DISPLAY_TIME = 6;
DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES+1];

float g_ColorBlue[3] = { 0.6, 0.8, 1.0 };
float g_ColorRed[3]	= { 1.0, 0.25, 0.25 };
float g_ColorGreen[3] = { 0.6, 1.0, 0.6 };
float g_ColorYellow[3] = { 1.0, 0.7, 0.0 };
float g_ColorGrey[3] = { 0.8, 0.8, 0.8 };

float *GetClientColor( int clientIndex )
{
	if( clientIndex > 0 && clientIndex <= MAX_PLAYERS )
	{
		const char *teamName = g_PlayerExtraInfo[clientIndex].teamname;
		if( !Q_stricmp( teamName, "red" ))
			return g_ColorRed;
		if( !Q_stricmp( teamName, "blue" ))
			return g_ColorBlue;
		if( !Q_stricmp( teamName, "spectator" ))
			return g_ColorGrey;
	}

	switch( g_PlayerExtraInfo[clientIndex].teamnumber )
	{
	case 1: return g_ColorRed;
	case 2: return g_ColorBlue;
	case 3: return g_ColorYellow;
	case 4: return g_ColorGreen;
	case 0: return g_ColorYellow;
	default: return g_ColorGrey;
	}
	return NULL;
}

DECLARE_MESSAGE( m_DeathNotice, DeathMsg );

int CHudDeathNotice :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( DeathMsg );

	CVAR_REGISTER( "hud_deathnotice_time", "6", 0 );

	return 1;
}

void CHudDeathNotice :: InitHUDData( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}

int CHudDeathNotice :: VidInit( void )
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );

	return 1;
}

int CHudDeathNotice :: Draw( float flTime )
{
	int x, y, r, g, b;

	for( int i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if( rgDeathNoticeList[i].iId == 0 )
			break; // we've gone through them all

		if( rgDeathNoticeList[i].flDisplayTime < flTime )
		{
			// display time has expired
			// remove the current item from the list
			memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i+1], sizeof( DeathNoticeItem ) * ( MAX_DEATHNOTICES - i ));
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = Q_min( rgDeathNoticeList[i].flDisplayTime, gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME );

		// Draw the death notice
		y = DEATHNOTICE_TOP + (20 * i);  //!!!

		int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
		x = ScreenWidth - ConsoleStringLen( rgDeathNoticeList[i].szVictim ) - ( gHUD.GetSpriteRect( id ).right - gHUD.GetSpriteRect( id ).left );

		if( !rgDeathNoticeList[i].iSuicide )
		{
			x -= (5 + ConsoleStringLen( rgDeathNoticeList[i].szKiller ));

			// Draw killers name
			x = 5 + DrawConsoleString( x, y, rgDeathNoticeList[i].szKiller );
		}

		r = 255;
		g = 80;
		b = 0;

		if( rgDeathNoticeList[i].iTeamKill )
		{
			// display it in sickly green
			r = 10;
			g = 240;
			b = 10;
		}

		// Draw death weapon
		SPR_Set( gHUD.GetSprite( id ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( id ));

		x += (gHUD.GetSpriteRect( id ).right - gHUD.GetSpriteRect( id ).left );

		// Draw victims name
		x = DrawConsoleString( x, y, rgDeathNoticeList[i].szVictim );
	}

	return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice :: MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );
	const int killer = READ_BYTE();
	const int victim = READ_BYTE();
	char killedwith[32];
	Q_strcpy( killedwith, "d_" );
	Q_strncat( killedwith, READ_STRING(), sizeof( killedwith ));
	const bool showKillFeed = READ_BYTE() != 0;

	gHUD.m_Scoreboard.GetAllPlayersInfo();
	const char *killerName = ( killer > 0 && killer < MAX_PLAYERS && g_PlayerInfoList[killer].name ) ? g_PlayerInfoList[killer].name : "";
	const bool nonPlayerKill = ((char)victim) == -1;
	const char *victimName = ( !nonPlayerKill && victim > 0 && victim < MAX_PLAYERS && g_PlayerInfoList[victim].name ) ? g_PlayerInfoList[victim].name : "";

	if( showKillFeed )
	{
		m_iFlags |= HUD_ACTIVE;
		int slot = 0;
		while( slot < MAX_DEATHNOTICES && rgDeathNoticeList[slot].iId != 0 ) ++slot;
		if( slot == MAX_DEATHNOTICES )
		{
			memmove( rgDeathNoticeList, rgDeathNoticeList + 1, sizeof( DeathNoticeItem ) * MAX_DEATHNOTICES );
			slot = MAX_DEATHNOTICES - 1;
		}
		memset( &rgDeathNoticeList[slot], 0, sizeof( rgDeathNoticeList[slot] ));
		Q_strncpy( rgDeathNoticeList[slot].szKiller, killerName, MAX_PLAYER_NAME_LENGTH );
		Q_strncpy( rgDeathNoticeList[slot].szVictim, nonPlayerKill ? killedwith + 2 : victimName, MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[slot].KillerColor = killer > 0 && killer < MAX_PLAYERS ? GetClientColor( killer ) : g_ColorYellow;
		rgDeathNoticeList[slot].VictimColor = victim > 0 && victim < MAX_PLAYERS ? GetClientColor( victim ) : g_ColorYellow;
		rgDeathNoticeList[slot].iNonPlayerKill = nonPlayerKill;
		rgDeathNoticeList[slot].iSuicide = !nonPlayerKill && ( killer == victim || killer == 0 );
		rgDeathNoticeList[slot].iTeamKill = !Q_strcmp( killedwith, "d_teammate" );
		rgDeathNoticeList[slot].iId = gHUD.GetSpriteIndex( killedwith );
		DEATHNOTICE_DISPLAY_TIME = CVAR_GET_FLOAT( "hud_deathnotice_time" );
		rgDeathNoticeList[slot].flDisplayTime = gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME;
	}
	else
	{
		memset( rgDeathNoticeList, 0, sizeof( rgDeathNoticeList ));
	}

	if( nonPlayerKill )
	{
		ConsolePrint( killerName ); ConsolePrint( " killed a " ); ConsolePrint( killedwith + 2 );
	}
	else if( killer == victim || killer == 0 )
	{
		ConsolePrint( victimName );
		ConsolePrint( !Q_strcmp( killedwith, "d_world" ) ? " died" : " killed self" );
	}
	else if( !Q_strcmp( killedwith, "d_teammate" ))
	{
		ConsolePrint( killerName ); ConsolePrint( " killed his teammate " ); ConsolePrint( victimName );
	}
	else
	{
		ConsolePrint( killerName ); ConsolePrint( " killed " ); ConsolePrint( victimName );
	}

	if( !nonPlayerKill && killer != 0 && Q_strcmp( killedwith, "d_world" ) && Q_strcmp( killedwith, "d_teammate" ))
	{
		ConsolePrint( " with " );
		if( !Q_strcmp( killedwith + 2, "egon" )) Q_strcpy( killedwith, "d_gluon gun" );
		if( !Q_strcmp( killedwith + 2, "gauss" )) Q_strcpy( killedwith, "d_tau cannon" );
		ConsolePrint( killedwith + 2 );
	}
	ConsolePrint( "\n" );

	END_READ();

	return 1;
}
