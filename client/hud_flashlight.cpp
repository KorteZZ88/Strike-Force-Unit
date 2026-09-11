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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "utils.h"
#include "parsemsg.h"
#include "triangleapi.h"

extern float g_flHealthIconRight;

DECLARE_MESSAGE( m_Flash, FlashBat )
DECLARE_MESSAGE( m_Flash, Flashlight )

int CHudFlashlight::Init( void) 
{
	m_fFade = 0;
	m_fOn = 0;
	m_iBat = 100;
	m_flBat = 1.0f;

	HOOK_MESSAGE( Flashlight );
	HOOK_MESSAGE( FlashBat );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem( this );

	return 1;
}

void CHudFlashlight::Reset( void )
{
	m_fFade = 0;
	m_fOn = 0;
	m_iBat = 100;
	m_flBat = 1.0f;
}

int CHudFlashlight::VidInit( void )
{
	m_hSprite1 = 0;
	m_prc1 = NULL;
	const int index = gHUD.GetSpriteIndex("flash_icon");
	if(index >= 0)
	{
		m_hSprite1 = gHUD.GetSprite(index);
		m_prc1 = &gHUD.GetSpriteRect(index);
	}
	return 1;
}
int CHudFlashlight:: MsgFunc_FlashBat( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	m_iBat = READ_BYTE();
	m_flBat = ((float)m_iBat ) / 100.0f;

	END_READ();

	return 1;
}

int CHudFlashlight:: MsgFunc_Flashlight( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	m_fOn = READ_BYTE();
	m_iBat = READ_BYTE();
	m_flBat = ((float)m_iBat ) / 100.0f;

	END_READ();

	return 1;
}

int CHudFlashlight::Draw( float flTime )
{
	if(!m_hSprite1 || !m_prc1 || (!m_fOn && m_iBat >= 100)) return 1;
	if((gHUD.m_iHideHUDDisplay & (HIDEHUD_FLASHLIGHT | HIDEHUD_ALL)) ||
		!gHUD.HasWeapon(WEAPON_SUIT) || gEngfuncs.IsSpectateOnly()) return 1;
	const int atlasWidth = SPR_Width(m_hSprite1, 0);
	const int atlasHeight = SPR_Height(m_hSprite1, 0);
	const wrect_t &rc = *m_prc1;
	if(atlasWidth <= 0 || atlasHeight <= 0 || rc.left < 0 || rc.top < 0 ||
		rc.right > atlasWidth || rc.bottom > atlasHeight ||
		rc.right <= rc.left || rc.bottom <= rc.top) return 1;
	model_t *model = (model_t *)gEngfuncs.GetSpritePointer(m_hSprite1);
	if(!model || !gEngfuncs.pTriAPI->SpriteTexture(model, 0)) return 1;
	const int height = 10;
	const float width = height * (float)(rc.right - rc.left) / (rc.bottom - rc.top);
	const float x = g_flHealthIconRight + 6, bottom = ScreenHeight - gHUD.m_iFontHeight / 2;
	const float u0 = (float)rc.left / atlasWidth, u1 = (float)rc.right / atlasWidth;
	const float v0 = (float)rc.top / atlasHeight, v1 = (float)rc.bottom / atlasHeight;
	const float empty = 1.0f - bound(0.0f, m_flBat, 1.0f);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	// As in the original flashlight HUD, charge occupies the right-hand part.
	for(int region = 0; region < 2; ++region)
	{
		const float start = region ? empty : 0.0f;
		const float end = region ? 1.0f : empty;
		if(end <= start) continue;
		const float shade = region ? (m_fOn ? 1.0f : 0.45f) : 0.12f;
		const float left = x + width * start, right = x + width * end;
		const float leftU = u0 + (u1 - u0) * start, rightU = u0 + (u1 - u0) * end;
		gEngfuncs.pTriAPI->Color4f(shade, shade, shade, 1);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		gEngfuncs.pTriAPI->TexCoord2f(leftU, v0); gEngfuncs.pTriAPI->Vertex3f(left, bottom - height, 0);
		gEngfuncs.pTriAPI->TexCoord2f(leftU, v1); gEngfuncs.pTriAPI->Vertex3f(left, bottom, 0);
		gEngfuncs.pTriAPI->TexCoord2f(rightU, v1); gEngfuncs.pTriAPI->Vertex3f(right, bottom, 0);
		gEngfuncs.pTriAPI->TexCoord2f(rightU, v0); gEngfuncs.pTriAPI->Vertex3f(right, bottom - height, 0);
		gEngfuncs.pTriAPI->End();
	}
	gEngfuncs.pTriAPI->Color4f(1, 1, 1, 1);
	gEngfuncs.pTriAPI->CullFace(TRI_FRONT);
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	return 1;
}
