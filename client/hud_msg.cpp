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
//  hud_msg.cpp
//
#include "hud.h"
#include "ammohistory.h"
#include "utils.h"
#include "parsemsg.h"
#include "gl_local.h"
#include "r_weather.h"
#include "r_efx.h"
#include "gl_studio.h"
#include "postfx_controller.h"
#include "gl_postprocess.h"

// CHud message handlers
DECLARE_HUDMESSAGE( Logo );
DECLARE_HUDMESSAGE( Weapons );
DECLARE_HUDMESSAGE( RainData );
DECLARE_HUDMESSAGE( SetBody );
DECLARE_HUDMESSAGE( SetSkin );
DECLARE_HUDMESSAGE( ResetHUD );
DECLARE_HUDMESSAGE( InitHUD );
DECLARE_HUDMESSAGE( ViewMode );
DECLARE_HUDMESSAGE( Particle );
DECLARE_HUDMESSAGE( KillPart );
DECLARE_HUDMESSAGE( SetFOV );
DECLARE_HUDMESSAGE( Concuss );
DECLARE_HUDMESSAGE( GameMode );
DECLARE_HUDMESSAGE( MusicFade );
DECLARE_HUDMESSAGE( WeaponAnim );
DECLARE_HUDMESSAGE( KillDecals );
DECLARE_HUDMESSAGE( CustomDecal );
DECLARE_HUDMESSAGE( StudioDecal );
DECLARE_HUDMESSAGE( SetupBones );
DECLARE_HUDMESSAGE( PostFxSettings );
DECLARE_HUDMESSAGE( Flashbang );
DECLARE_HUDMESSAGE( GasEffect );
DECLARE_HUDMESSAGE( NVGOwned );
DECLARE_HUDMESSAGE( NVGSound );
DECLARE_HUDMESSAGE( SpecTarget );
DECLARE_HUDMESSAGE( VModelSound );

int CHud :: InitHUDMessages( void )
{
	HOOK_MESSAGE( Logo );
	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( Concuss );
	HOOK_MESSAGE( Weapons );
	HOOK_MESSAGE( Particle );
	HOOK_MESSAGE( KillPart );
	HOOK_MESSAGE( RainData ); 
	HOOK_MESSAGE( SetBody );
	HOOK_MESSAGE( SetSkin );
	HOOK_MESSAGE( MusicFade );
	HOOK_MESSAGE( WeaponAnim );
	HOOK_MESSAGE( KillDecals );
	HOOK_MESSAGE( CustomDecal );
	HOOK_MESSAGE( StudioDecal );
	HOOK_MESSAGE( SetupBones );
	HOOK_MESSAGE( PostFxSettings );
	HOOK_MESSAGE( Flashbang );
	HOOK_MESSAGE( GasEffect );
	HOOK_MESSAGE( NVGOwned );
	HOOK_MESSAGE( NVGSound );
	HOOK_MESSAGE( SpecTarget );
	HOOK_MESSAGE( VModelSound );

	m_iFOV = 0;
	
	CVAR_REGISTER( "zoom_sensitivity_ratio", "1.2", 0 );
	default_fov = CVAR_REGISTER( "default_fov", "90", 0 );
	m_pCvarDraw = CVAR_REGISTER( "hud_draw", "1", FCVAR_ARCHIVE );
	m_pCvarColorRed = CVAR_REGISTER( "hud_color_red", "255", FCVAR_ARCHIVE );
	m_pCvarColorGreen = CVAR_REGISTER( "hud_color_green", "160", FCVAR_ARCHIVE );
	m_pCvarColorBlue = CVAR_REGISTER( "hud_color_blue", "0", FCVAR_ARCHIVE );
	m_pCvarTeamColor = CVAR_REGISTER( "hud_teamcolor", "1", FCVAR_ARCHIVE );
	m_pSpriteList = NULL;

	// clear any old HUD list
	if( m_pHudList )
	{
		HUDLIST *pList;

		while( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	m_flTime = 1.0;
	m_flFlashbangEffectStart = -1.0f;
	m_iFlashbangEffectAlpha = 0;
	m_iFlashbangStunAlpha = 0;
	m_iFlashbangEffectGeneration = 1;
	m_flGasStrength = 0;
	m_bGasDelayedEffects = false;
	m_iGasEffectGeneration = 1;

	return 1;
}

int CHud::MsgFunc_VModelSound(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pszName, pbuf, iSize);
	const int source = READ_BYTE();
	Vector origin(READ_COORD(), READ_COORD(), READ_COORD());
	const char* sample = READ_STRING();
	cl_entity_t* local = gEngfuncs.GetLocalPlayer();
	const bool owner = local && local->index == source;
	const float volume = owner ? 1.0f : 0.6f;
	const bool gunshot = Q_stristr(sample, "fire") != NULL || Q_stristr(sample, "shot") != NULL;
	const float attenuation = owner ? ATTN_NORM : (gunshot ? ATTN_NORM / 2.0f : ATTN_NORM / 0.6f);
	gEngfuncs.pEventAPI->EV_PlaySound(source, origin, CHAN_ITEM, sample, volume, attenuation, 0, PITCH_NORM);
	return 1;
}

int CHud::MsgFunc_NVGOwned(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pszName,pbuf,iSize);
	SetNightVisionOwned(READ_BYTE()!=0);
	END_READ();
	return 1;
}

int CHud::MsgFunc_NVGSound(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pszName,pbuf,iSize);
	const int source = READ_BYTE();
	const bool enabled = READ_BYTE() != 0;
	const float volume = READ_BYTE() / 255.0f;
	END_READ();
	cl_entity_t *entity = gEngfuncs.GetEntityByIndex(source);
	if (entity)
		gEngfuncs.pEventAPI->EV_PlaySound(source, entity->origin, CHAN_ITEM,
			enabled ? "items/NVGon.wav" : "items/NVGoff.wav", volume, ATTN_NORM, 0, PITCH_NORM);
	return 1;
}

int CHud::MsgFunc_SpecTarget(const char *pszName,int iSize,void *pbuf)
{
	BEGIN_READ(pszName,pbuf,iSize);
	m_bInEyeSpectator=READ_BYTE()!=0;
	m_iSpectatorTarget=READ_BYTE();
	END_READ();
	return 1;
}

int CHud :: MsgFunc_ResetHUD( const char *pszName, int iSize, void *pbuf )
{
	m_bInEyeSpectator=false;m_iSpectatorTarget=0;
	SetNightVisionOwned(false);
	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );

	m_ViewSmoothingData.bClampEyeAngles = true;

	m_ViewSmoothingData.flPitchCurveZero = PITCH_CURVE_ZERO;
	m_ViewSmoothingData.flPitchCurveLinear = PITCH_CURVE_LINEAR;
	m_ViewSmoothingData.flRollCurveZero = ROLL_CURVE_ZERO;
	m_ViewSmoothingData.flRollCurveLinear = ROLL_CURVE_LINEAR;

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	// The client DLL survives map restarts while server time starts over. Do
	// not carry placement-preview attack suppression into the new map.
	m_iBuildPreviewState = 0;
	m_flBuildPreviewPendingUntil = 0.0f;
	m_flBuildPreviewSeenTime = -1.0f;
	m_bSuppressBuildAttackUntilRelease = false;
	m_flFlashbangEffectStart = -1.0f;
	m_iFlashbangEffectAlpha = 0;
	m_iFlashbangStunAlpha = 0;
	++m_iFlashbangEffectGeneration;
	m_flGasStrength = 0;
	m_bGasDelayedEffects = false;
	++m_iGasEffectGeneration;

	return 1;
}

int CHud::MsgFunc_Flashbang(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pszName, pbuf, iSize);
	m_iFlashbangEffectAlpha = READ_BYTE();
	m_iFlashbangStunAlpha = READ_BYTE();
	if( m_iFlashbangEffectAlpha == 0 && m_iFlashbangStunAlpha == 0 )
	{
		m_flFlashbangEffectStart = -1.0f;
		++m_iFlashbangEffectGeneration;
		screenfade_t fade = {};
		fade.fadeReset = gEngfuncs.GetClientTime();
		fade.fadeEnd = gEngfuncs.GetClientTime();
		gEngfuncs.pfnSetScreenFade(&fade);
		return 1;
	}
	m_flFlashbangEffectStart = gEngfuncs.GetClientTime();
	return 1;
}

int CHud::MsgFunc_GasEffect(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pszName,pbuf,iSize);
	m_flGasStrength=READ_BYTE()/255.0f;
	const bool wasDelayed = m_bGasDelayedEffects;
	m_bGasDelayedEffects=READ_BYTE()!=0;
	if( m_flGasStrength <= 0.0f )
		++m_iGasEffectGeneration;
	if( m_bGasDelayedEffects )
		m_Health.TriggerGasDamageIndicators();
	else if( wasDelayed )
		m_Health.ClearGasDamageIndicators();
	return 1;
}

int CHud :: MsgFunc_Logo( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	// update logo data
	m_iLogo = READ_BYTE();

	END_READ();

	return 1;
}

int CHud :: MsgFunc_Weapons( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	// update weapon bits
	READ_BYTES( m_iWeaponBits, MAX_WEAPON_BYTES );

	// Apply ownership immediately.  Deferring this entirely to CHudAmmo::Think
	// can leave valid weapons out of rgSlots until an unrelated weapon change.
	for( int id = 1; id < MAX_WEAPONS; ++id )
	{
		WEAPON *weapon = gWR.GetWeapon( id );
		if( !weapon || !weapon->iId )
			continue;
		if( HasWeapon( id ))
			gWR.PickupWeapon( weapon );
		else
			gWR.DropWeapon( weapon );
	}
	memcpy( gWR.iOldWeaponBits, m_iWeaponBits, MAX_WEAPON_BYTES );

	END_READ();

	return 1;
}

int CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	return 1;
}

int CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	m_iBuildPreviewState = 0;
	m_flBuildPreviewPendingUntil = 0.0f;
	m_flBuildPreviewSeenTime = -1.0f;
	m_bSuppressBuildAttackUntilRelease = false;

	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	return 1;
}

int CHud::MsgFunc_SetFOV( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	int newfov = READ_BYTE();
	float def_fov = CVAR_GET_FLOAT( "default_fov" );

	if (newfov == 0) {
		m_iFOV = def_fov;
		m_zoomMode = false;
	}
	else {
		m_iFOV = newfov;
		m_zoomMode = true;
	}

	if( m_iFOV == def_fov )
	{
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{
		m_flMouseSensitivity = CVAR_GET_FLOAT( "sensitivity" );

		// scale by zoom ratio
		m_flMouseSensitivity *= ((float)newfov / def_fov) * CVAR_GET_FLOAT( "zoom_sensitivity_ratio" );
	}

	END_READ();

	return 1;
}

int CHud :: MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	m_Teamplay = READ_BYTE();

	END_READ();
	
	return 1;
}

int CHud :: MsgFunc_Damage( const char *pszName, int iSize, void *pbuf )
{
	int	armor, blood;
	Vector	from;
	float	count;

	BEGIN_READ( pszName, pbuf, iSize );

	armor = READ_BYTE();
	blood = READ_BYTE();

	for( int i = 0; i < 3; i++ )
		from[i] = READ_COORD();

	count = (blood * 0.5f) + (armor * 0.5f);
	if( count < 10 ) count = 10;

	END_READ();
	
	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	m_iConcussionEffect = READ_BYTE();

	if( m_iConcussionEffect )
		m_StatusIcons.EnableIcon("dmg_concuss", 255, 160, 0 );
	else m_StatusIcons.DisableIcon( "dmg_concuss" );

	END_READ();

	return 1;
}

int CHud :: MsgFunc_SetBody( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	gEngfuncs.GetViewModel()->curstate.body = READ_BYTE();

	END_READ();
	
	return 1;
}

int CHud :: MsgFunc_SetSkin( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	gEngfuncs.GetViewModel()->curstate.skin = READ_BYTE();

	END_READ();
	
	return 1;
}

int CHud :: MsgFunc_WeaponAnim( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	int sequence = READ_BYTE();
	int body = READ_BYTE();
	float framerate = READ_BYTE() * 0.125f;

	END_READ();

	cl_entity_t *view = gEngfuncs.GetViewModel();

	// call weaponanim with same body
	gEngfuncs.pfnWeaponAnim( sequence, body );

	if( framerate > 0.0f )
		view->curstate.framerate = framerate;
	else view->curstate.framerate = 1.0f;
#if 0
	// just for test Glow Shell effect
	view->curstate.renderfx = kRenderFxGlowShell;
	view->curstate.rendercolor.r = 255;
	view->curstate.rendercolor.g = 255;
	view->curstate.rendercolor.b = 255;
	view->curstate.renderamt = 100;
#endif	
	return 1;
}

int CHud :: MsgFunc_Particle( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	int entindex = READ_SHORT();
	char *sz = READ_STRING();
	int attachment = READ_BYTE();

	COM_DefaultExtension( sz, ".aur" );

	UTIL_CreateAurora( GET_ENTITY( entindex ), sz, attachment );

	END_READ();
	
	return 1;
}

int CHud :: MsgFunc_KillPart( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );
	int entindex = READ_SHORT();

	UTIL_RemoveAurora( GET_ENTITY( entindex ));

	END_READ();
	
	return 1;
}

int CHud :: MsgFunc_KillDecals( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	int entityIndex = READ_SHORT();
	if( g_fRenderInitialized && entityIndex == 0 )
	{
		g_StudioRenderer.StudioClearDecals();
		END_READ();
		return 1;
	}

	if( g_fRenderInitialized )
		g_StudioRenderer.RemoveAllDecals( entityIndex );

	cl_entity_t *ent = gEngfuncs.GetEntityByIndex( entityIndex );

	if( g_fRenderInitialized && ent && ent->model && ent->model->type == mod_brush )
	{
		REMOVE_BSP_DECALS( ent->model );
	}

	END_READ();
	
	return 1;
}

int CHud::MsgFunc_CustomDecal(const char *pszName, int iSize, void *pbuf)
{
	char name[80];

	BEGIN_READ(pszName, pbuf, iSize);

	Vector pos, normal;
	pos.x = READ_COORD();
	pos.y = READ_COORD();
	pos.z = READ_COORD();
	normal.x = READ_COORD() / 8192.0f;
	normal.y = READ_COORD() / 8192.0f;
	normal.z = READ_COORD() / 8192.0f;
	int entityIndex = READ_SHORT();
	int modelIndex = READ_SHORT();
	Q_strncpy(name, READ_STRING(), sizeof(name));
	int flags = READ_BYTE();
	float angle = READ_ANGLE();

	CreateDecal(pos, normal, angle, name, flags, entityIndex, modelIndex);

	return 1;
}

int CHud :: MsgFunc_StudioDecal( const char *pszName, int iSize, void *pbuf )
{
	Vector vecEnd, vecNormal, vecScale = g_vecZero;
	char name[80];

	BEGIN_READ(pszName, pbuf, iSize);

	vecEnd.x = READ_COORD();
	vecEnd.y = READ_COORD();
	vecEnd.z = READ_COORD();
	vecNormal.x = READ_COORD() * 0.001f;
	vecNormal.y = READ_COORD() * 0.001f;
	vecNormal.z = READ_COORD() * 0.001f;
	int entityIndex = READ_SHORT();
	int modelIndex = READ_SHORT();
	Q_strncpy(name, READ_STRING(), sizeof(name));
	int flags = READ_BYTE();

	modelstate_t state;
	state.sequence = READ_SHORT();
	state.frame = READ_SHORT();
	state.blending[0] = READ_BYTE();
	state.blending[1] = READ_BYTE();
	state.controller[0] = READ_BYTE();
	state.controller[1] = READ_BYTE();
	state.controller[2] = READ_BYTE();
	state.controller[3] = READ_BYTE();
	state.body = READ_BYTE();
	state.skin = READ_BYTE();
	int cacheID = READ_SHORT();

	if (REMAIN_BYTES())
	{
		vecScale.x = READ_COORD() * 0.001f;
		vecScale.y = READ_COORD() * 0.001f;
		vecScale.z = READ_COORD() * 0.001f;
	}

	cl_entity_t *ent = GET_ENTITY(entityIndex);

	if (!ent)
	{
		// something very bad happens...
		ALERT(at_error, "StudioDecal: ent == NULL\n");
		return 1;
	}

	g_StudioRenderer.PushEntityState(ent);
	g_StudioRenderer.ModelStateToEntity(ent, &state);

	// restore model in case decalmessage was delivered early than delta-update
	if (!ent->model && modelIndex != 0)
		ent->model = IEngineStudio.GetModelByIndex(modelIndex);

	if (cacheID)
	{
		// tell the code about vertex lighting
		SetBits(ent->curstate.iuser1, CF_STATIC_ENTITY);
		ent->curstate.colormap = cacheID;
	}

	if (!RENDER_GET_PARM(PARM_CLIENT_ACTIVE, 0) && FBitSet(ent->curstate.iuser1, CF_STATIC_ENTITY))
		ent->curstate.startpos = vecScale; // restore scale here

	if (!ent->model || ent->model->type != mod_studio)
		return 1;

	g_StudioRenderer.StudioDecalShoot(vecNormal, vecEnd, name, ent, flags, &state);
	g_StudioRenderer.PopEntityState(ent);

	return 1;
}

int CHud :: MsgFunc_SetupBones( const char *pszName, int iSize, void *pbuf )
{
	static Vector pos[MAXSTUDIOBONES];
	static Radian ang[MAXSTUDIOBONES];

	BEGIN_READ(pszName, pbuf, iSize);
		int entityIndex = READ_SHORT();
		int boneCount = READ_BYTE();
		for (int i = 0; i < boneCount; i++)
		{
			pos[i].x = (float)READ_SHORT() * (1.0f/128.0f);
			pos[i].y = (float)READ_SHORT() * (1.0f/128.0f);
			pos[i].z = (float)READ_SHORT() * (1.0f/128.0f);
			ang[i].x = (float)READ_SHORT() * (1.0f/512.0f);
			ang[i].y = (float)READ_SHORT() * (1.0f/512.0f);
			ang[i].z = (float)READ_SHORT() * (1.0f/512.0f);
		}
	END_READ();	

	cl_entity_t *ent = GET_ENTITY(entityIndex);
	if (!ent)
	{
		// something very bad happens...
		ALERT(at_error, "MsgFunc_SetupBones: ent == NULL\n");
		return 1;
	}

	R_StudioSetBonesExternal(ent, pos, ang);
	return 1;
}

int _cdecl CHud::MsgFunc_PostFxSettings(const char * pszName, int iSize, void * pbuf)
{
	CPostFxParameters postFxParams;
	BEGIN_READ(pszName, pbuf, iSize);
	postFxParams.ParseMessage();
	g_PostFxController.UpdateState(postFxParams);
	END_READ();	
	return 1;
}

int CHud :: MsgFunc_RainData( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	if( g_fRenderInitialized )
		R_ParseRainMessage();

	END_READ();	

	return 1;
}

int CHud :: MsgFunc_MusicFade( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	if( g_fRenderInitialized )
		MUSIC_FADE_VOLUME( (float)READ_SHORT() / 100.0f );

	END_READ();

	return 1;
}
