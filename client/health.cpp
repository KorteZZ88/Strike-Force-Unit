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
// Health.cpp
//
// implementation of CHudHealth class
//

#include "hud.h"
#include "utils.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include <vector>

DECLARE_MESSAGE( m_Health, Health )
DECLARE_MESSAGE( m_Health, Damage )
DECLARE_MESSAGE( m_Health, GasDamage )
DECLARE_MESSAGE( m_Health, MoneyDelta )

#define PAIN_NAME	"sprites/640_pain.spr"
#define DAMAGE_NAME	"sprites/%d_dmg.spr"

// File-local resource: adding the icon must not change the CHudHealth/CHud layout.
static SpriteHandle s_hSoldier = 0;
struct ArmorSpan { int x, y, alpha, borderAlpha; };
static std::vector<ArmorSpan> s_armorOutline;
static int s_armorWidth = 0, s_armorHeight = 0;

// Build a two-screen-pixel outline from the actual sprite, once per size/video reset.
static void BuildArmorOutline(int width, int height)
{
	s_armorWidth = width; s_armorHeight = height;
	s_armorOutline.clear();
	int length = 0;
	byte *data = gEngfuncs.COM_LoadFile("sprites/Soldier.spr", 5, &length);
	if(!data) return;
	auto read = [](const byte *p) -> unsigned int {
		return p[0] | (unsigned int)p[1] << 8 | (unsigned int)p[2] << 16 | (unsigned int)p[3] << 24;
	};
	if(length >= 42 && read(data) == 0x50534449 && read(data + 4) == 2 && read(data + 12) == 1)
	{
		const unsigned int colors = data[40] | (unsigned int)data[41] << 8;
		const unsigned int frame = 42 + colors * 3;
		if(colors > 0 && colors <= 256 && frame + 20 <= (unsigned int)length && read(data + frame) == 0)
		{
			const unsigned int sw = read(data + frame + 12), sh = read(data + frame + 16);
			if(sw && sh && sw <= 16384 && sh <= 16384 && sh <= ((unsigned int)length - frame - 20) / sw)
			{
				const int samples = 4;
				width *= samples;
				height *= samples;
				const int cropped = (width * 217 + 511) / 512;
				std::vector<byte> mask(cropped * height, 0);
				for(int y = 0; y < height; ++y)
					for(int x = 0; x < cropped; ++x)
					{
						const unsigned int sx = Q_min(sw - 1, (unsigned int)((147.0f / 512.0f + (x + 0.5f) / width) * sw));
						const unsigned int sy = Q_min(sh - 1, (unsigned int)((y + 0.5f) * sh / height));
						const unsigned int index = data[frame + 20 + sy * sw + sx];
						if(index < colors)
						{
							const byte *rgb = data + 42 + index * 3;
							mask[y * cropped + x] = Q_max(rgb[0], Q_max(rgb[1], rgb[2])) > 96;
						}
					}
				auto inside = [&](int x, int y) { return x >= 0 && x < cropped && y >= 0 && y < height && mask[y * cropped + x]; };
				for(int y = -20; y < height + 20; y += samples)
				{
					for(int x = -20; x < cropped + 20; x += samples)
					{
						int ring = 0, backing = 0;
						for(int sy = 0; sy < samples; ++sy)
							for(int sx = 0; sx < samples; ++sx)
							{
								if(inside(x + sx, y + sy)) continue;
								int distance2 = 325;
								for(int dy = -18; dy <= 18; ++dy)
									for(int dx = -18; dx <= 18; ++dx)
										if(dx * dx + dy * dy < distance2 && inside(x + sx + dx, y + sy + dy)) distance2 = dx * dx + dy * dy;
								if(distance2 <= 324) ++backing;
								// 1.5-pixel separator and the same 2.5-pixel blue ring.
								if(distance2 > 36 && distance2 <= 256) ++ring;
							}
						if(backing) s_armorOutline.push_back({x / samples, y / samples, ring * 255 / 16, backing * 230 / 16});
					}
				}
			}
		}
	}
	gEngfuncs.COM_FreeFile(data);
}
float g_flHealthIconRight = 12.0f;

int giDmgHeight, giDmgWidth;

int giDmgFlags[NUM_DMG_TYPES] = 
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS, 
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_NUCLEAR
};

int CHudHealth::Init( void )
{
	HOOK_MESSAGE( Health );
	HOOK_MESSAGE( Damage );
	HOOK_MESSAGE( GasDamage );
	HOOK_MESSAGE( MoneyDelta );
	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_flGasIndicatorUntil = 0;
	m_iMoneyDelta = 0;
	m_flMoneyDeltaUntil = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset( m_dmg, 0, sizeof( m_dmg ));

	gHUD.AddHudElem( this );

	return 1;
}

void CHudHealth::Reset( void )
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;

	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	m_flGasIndicatorUntil = 0;

	for( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}
}

int CHudHealth::VidInit( void )
{
	m_hSprite = 0;
	s_hSoldier = SPR_Load( "sprites/Soldier.spr" );
	s_armorWidth = s_armorHeight = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );

	giDmgHeight = gHUD.GetSpriteRect( m_HUD_dmg_bio ).right - gHUD.GetSpriteRect( m_HUD_dmg_bio ).left;
	giDmgWidth = gHUD.GetSpriteRect( m_HUD_dmg_bio ).bottom - gHUD.GetSpriteRect( m_HUD_dmg_bio ).top;
	return 1;
}

int CHudHealth:: MsgFunc_Health( const char *pszName,  int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pszName, pbuf, iSize );

	int x = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if( x != m_iHealth )
	{
		m_fFade = FADE_TIME;
		m_iHealth = x;
	}

	END_READ();

	return 1;
}

int CHudHealth:: MsgFunc_Damage(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pszName, pbuf, iSize );

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();// health
	long bitsDamage = READ_LONG();// damage bits

	Vector vecFrom;

	for( int i = 0 ; i < 3 ; i++)
		vecFrom[i] = READ_COORD();

	// Toxic grenade damage has its own server-side flag so it can bypass armor
	// without activating Half-Life's built-in time-based poison damage.  On the
	// HUD it deliberately uses the nerve-gas (gas-mask) warning icon.
	if( bitsDamage & DMG_GAS_IGNORE_ARMOR )
	{
		bitsDamage |= DMG_NERVEGAS;
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1.0f;
	}

	UpdateTiles( gHUD.m_flTime, bitsDamage );

	// Actually took damage?
	if(( damageTaken > 0 || armor > 0 ) && !( bitsDamage & DMG_GAS_IGNORE_ARMOR ))
		CalcDamageDirection( vecFrom );

	END_READ();

	return 1;
}

int CHudHealth::MsgFunc_GasDamage(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pszName, pbuf, iSize);
	TriggerGasDamageIndicators();
	END_READ();
	return 1;
}

int CHudHealth::MsgFunc_MoneyDelta(const char* pszName,int iSize,void* pbuf)
{
	BEGIN_READ(pszName,pbuf,iSize);m_iMoneyDelta=READ_LONG();END_READ();
	m_flMoneyDeltaUntil=gHUD.m_flTime+3.0f;
	return 1;
}

void CHudHealth::TriggerGasDamageIndicators()
{
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1.0f;
	m_flGasIndicatorUntil = gHUD.m_flTime + 0.35f;
	UpdateTiles(gHUD.m_flTime, DMG_NERVEGAS);
}

void CHudHealth::ClearGasDamageIndicators()
{
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0.0f;
	m_flGasIndicatorUntil = 0.0f;
	m_bitsDamage &= ~DMG_NERVEGAS;
	m_dmg[DMG_IMAGE_NERVE].fExpire = 0.0f;
	m_dmg[DMG_IMAGE_NERVE].fBaseline = 0.0f;
	m_dmg[DMG_IMAGE_NERVE].x = m_dmg[DMG_IMAGE_NERVE].y = 0;
}

// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor( int &r, int &g, int &b )
{
	if(gHUD.m_pCvarTeamColor&&gHUD.m_pCvarTeamColor->value!=0)
	{
		r=gHUD.m_color.r;g=gHUD.m_color.g;b=gHUD.m_color.b;
		return;
	}
	int iHealth = m_iHealth;

	if( iHealth > 25)
		iHealth -= 25;
	else if( iHealth < 0 )
		iHealth = 0;

	if( m_iHealth > 25 )
	{
		r = gHUD.m_color.r;
		g = gHUD.m_color.g;
		b = gHUD.m_color.b;
	}
	else
	{
		r = 250;
		g = 0;
		b = 0;
	}
}

static bool DrawSoldier( int x, int bottom, int health )
{
	if( !s_hSoldier )
		return false;
	const int sourceWidth = SPR_Width( s_hSoldier, 0 );
	const int sourceHeight = SPR_Height( s_hSoldier, 0 );
	model_t *model = (model_t *)gEngfuncs.GetSpritePointer( s_hSoldier );
	if( !model || sourceWidth <= 0 || sourceHeight <= 0 ||
		!gEngfuncs.pTriAPI->SpriteTexture( model, 0 ))
		return false;

	// Fit the 100-pixel icon before the armor column, including low resolutions.
	const int height = Q_max( 1, Q_min( 100, (ScreenWidth / 5 - x - 8) * sourceHeight / sourceWidth ));
	const int width = Q_max( 1, height * sourceWidth / sourceHeight );
	// Align the visible silhouette, excluding Soldier.spr's empty side margins.
	const float leftUV = 147.0f / 512.0f, rightUV = 364.0f / 512.0f;
	const float drawX = 12.0f;
	g_flHealthIconRight = drawX + width * (rightUV - leftUV);
	const int top = bottom - height;
	// Round up so a living player always retains at least one of ten steps.
	const int steps = (bound( 0, health, 100 ) + 9) / 10;
	const float empty = (10 - steps) / 10.0f;
	const float split = top + height * empty;

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	// Separate regions avoid adding the dim silhouette to the white fill.
	for( int region = 0; region < 2; ++region )
	{
		const float v0 = region ? empty : 0.0f;
		const float v1 = region ? 1.0f : empty;
		if( v1 <= v0 )
			continue;
		const float y0 = region ? split : top;
		const float y1 = region ? bottom : split;
		const float red = region ? 1.0f : 0.28f;
		const float greenBlue = region ? 1.0f : 0.025f;
		gEngfuncs.pTriAPI->Color4f( red, greenBlue, greenBlue, 1.0f );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		gEngfuncs.pTriAPI->TexCoord2f( leftUV, v0 );
		gEngfuncs.pTriAPI->Vertex3f( drawX, y0, 0 );
		gEngfuncs.pTriAPI->TexCoord2f( leftUV, v1 );
		gEngfuncs.pTriAPI->Vertex3f( drawX, y1, 0 );
		gEngfuncs.pTriAPI->TexCoord2f( rightUV, v1 );
		gEngfuncs.pTriAPI->Vertex3f( g_flHealthIconRight, y1, 0 );
		gEngfuncs.pTriAPI->TexCoord2f( rightUV, v0 );
		gEngfuncs.pTriAPI->Vertex3f( g_flHealthIconRight, y0, 0 );
		gEngfuncs.pTriAPI->End();
	}
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1 );
	gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	if(s_armorWidth != width || s_armorHeight != height) BuildArmorOutline(width, height);
	const float armorSplit = top - 5 + (height + 10) * (1.0f - bound(0, gHUD.m_Battery.ArmorValue(), 100) / 100.0f);
	for(const ArmorSpan &span : s_armorOutline)
	{
		const bool charged = top + span.y + 0.5f >= armorSplit;
		if(!charged)
			continue;
		if(span.borderAlpha > 0)
			gEngfuncs.pfnFillRGBABlend((int)drawX + span.x, top + span.y, 1, 1, 0, 0, 0, Q_min(255, span.borderAlpha * 255 / 230));
		if(span.alpha > 0)
			gEngfuncs.pfnFillRGBABlend((int)drawX + span.x, top + span.y, 1, 1,
				40, 100, 255, span.alpha);
	}
	return true;
}

int CHudHealth::Draw( float flTime )
{
	int r, g, b;
	int a = 0, x, y;
	int HealthWidth;

	if(( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) || gEngfuncs.IsSpectateOnly() )
		return 1;

	if( !m_hSprite )
		m_hSprite = LoadSprite( PAIN_NAME );
	
	// Has health changed? Flash the health #
	if( m_fFade )
	{
		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if( m_fFade <= 0 )
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}

		// Fade the health number back to dim
		a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;

	}
	else
	{
		a = MIN_ALPHA;
	}
	
	// If health is getting low, make it bright red
	if( m_iHealth <= 15 )
		a = 255;
		
	GetPainColor( r, g, b );
	ScaleColors( r, g, b, a );

	// Only draw health if we have the suit.
	if (gHUD.HasWeapon( WEAPON_SUIT ))
	{
		HealthWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
		int CrossWidth = gHUD.GetSpriteRect( m_HUD_cross ).right - gHUD.GetSpriteRect( m_HUD_cross ).left;

		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		x = CrossWidth /2;

		const bool drewSoldier = DrawSoldier( x, ScreenHeight - gHUD.m_iFontHeight / 2, m_iHealth );
		if( !drewSoldier )
		{
			// Keep health readable in mods that do not supply the soldier sprite.
			SPR_Set( gHUD.GetSprite(m_HUD_cross ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( m_HUD_cross ));
			gHUD.DrawHudNumber( CrossWidth + HealthWidth / 2, y,
				DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, r, g, b );
		}
		x = CrossWidth + HealthWidth / 2 + 3 * HealthWidth;

		x += HealthWidth / 2;

		int iHeight = gHUD.m_iFontHeight;
		int iWidth = HealthWidth / 10;

		r = gHUD.m_color.r;
		g = gHUD.m_color.g;
		b = gHUD.m_color.b;
		if( !drewSoldier )
			FillRGBA( x, y, iWidth, iHeight, r, g, b, a );

		cl_entity_t *local=gEngfuncs.GetLocalPlayer();
		if(local&&local->index>0&&local->index<=MAX_PLAYERS&&g_PlayerExtraInfo[local->index].money>=0)
		{
			char money[16];Q_snprintf(money,sizeof(money),"%d",g_PlayerExtraInfo[local->index].money);
			int moneyDigits=(int)Q_strlen(money);int moneyX=ScreenWidth-12-moneyDigits*HealthWidth;
			int moneyY=ScreenHeight-96-gHUD.m_iFontHeight;
			int moneyR=gHUD.m_color.r,moneyG=gHUD.m_color.g,moneyB=gHUD.m_color.b;ScaleColors(moneyR,moneyG,moneyB,MIN_ALPHA);
			int dollarIndex=gHUD.GetSpriteIndex("dollar");
			if(dollarIndex>=0)
			{
				wrect_t &dollarRect=gHUD.GetSpriteRect(dollarIndex);
				int dollarWidth=dollarRect.right-dollarRect.left;
				SPR_Set(gHUD.GetSprite(dollarIndex),moneyR,moneyG,moneyB);
				SPR_DrawAdditive(0,moneyX-dollarWidth,moneyY,&dollarRect);
			}
			else
				gHUD.DrawHudString(moneyX-HealthWidth,moneyY,ScreenWidth,"$",moneyR,moneyG,moneyB);
			for(const char*digit=money;*digit;digit++)
			{
				int value=*digit-'0';SPR_Set(gHUD.GetSprite(gHUD.m_HUD_number_0+value),moneyR,moneyG,moneyB);
				SPR_DrawAdditive(0,moneyX,moneyY,&gHUD.GetSpriteRect(gHUD.m_HUD_number_0+value));moneyX+=HealthWidth;
			}
			if(m_flMoneyDeltaUntil>gHUD.m_flTime&&m_iMoneyDelta)
			{
				char delta[16];Q_snprintf(delta,sizeof(delta),"%u",m_iMoneyDelta<0?(unsigned)(-(long long)m_iMoneyDelta):(unsigned)m_iMoneyDelta);float remaining=m_flMoneyDeltaUntil-gHUD.m_flTime;int alpha=remaining>1.0f?255:bound(0,(int)(remaining*255.0f),255);int dr=m_iMoneyDelta>0?80:255,dg=m_iMoneyDelta>0?255:80,db=80;ScaleColors(dr,dg,db,alpha);
				int deltaDigits=(int)Q_strlen(delta);int deltaX=ScreenWidth-12-deltaDigits*HealthWidth,deltaY=moneyY-gHUD.m_iFontHeight-2;gHUD.DrawHudString(deltaX-HealthWidth,deltaY,ScreenWidth,m_iMoneyDelta>0?"+":"-",dr,dg,db);
				for(const char*digit=delta;*digit;digit++){int value=*digit-'0';SPR_Set(gHUD.GetSprite(gHUD.m_HUD_number_0+value),dr,dg,db);SPR_DrawAdditive(0,deltaX,deltaY,&gHUD.GetSpriteRect(gHUD.m_HUD_number_0+value));deltaX+=HealthWidth;}
			}
		}
	}

	DrawDamage( flTime );
	return DrawPain( flTime );
}

void CHudHealth::CalcDamageDirection( Vector vecFrom )
{
	Vector	forward, right, up;
	float	side, front;
	Vector	vecOrigin, vecAngles;

	if( vecFrom == g_vecZero )
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}

	vecOrigin = gHUD.m_vecOrigin;
	vecAngles = gHUD.m_vecAngles;

	vecFrom -= vecOrigin;

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors( vecAngles, forward, right, up );

	front = DotProduct( vecFrom, right );
	side = DotProduct( vecFrom, forward );

	if( flDistToTarget <= 50.0f )
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else 
	{
		if( side > 0.0f )
		{
			if( side > 0.3f )
				m_fAttackFront = Q_max( m_fAttackFront, side );
		}
		else
		{
			float f = fabs( side );
			if( f > 0.3f )
				m_fAttackRear = Q_max( m_fAttackRear, f );
		}

		if( front > 0.0f )
		{
			if( front > 0.3f )
				m_fAttackRight = Q_max( m_fAttackRight, front );
		}
		else
		{
			float f = fabs( front );
			if( f > 0.3f )
				m_fAttackLeft = Q_max( m_fAttackLeft, f );
		}
	}
}

int CHudHealth::DrawPain( float flTime )
{
	if( !( m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight ))
		return 1;

	int r, g, b;
	int x, y, a, shade;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;
	
	// SPR_Draw top
	if( m_fAttackFront > 0.4f )
	{
		if( flTime < m_flGasIndicatorUntil ) { r = 255; g = 0; b = 0; } else GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackFront, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 0 ) / 2;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite, 0 ) * 3;
		SPR_DrawAdditive( 0, x, y, NULL );
		m_fAttackFront = Q_max( 0, m_fAttackFront - fFade );
	}
	else
	{
		m_fAttackFront = 0;
	}

	if( m_fAttackRight > 0.4f )
	{
		if( flTime < m_flGasIndicatorUntil ) { r = 255; g = 0; b = 0; } else GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackRight, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 + SPR_Width( m_hSprite, 1 ) * 2;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite, 1 ) / 2;
		SPR_DrawAdditive( 1, x, y, NULL );
		m_fAttackRight = Q_max( 0, m_fAttackRight - fFade );
	}
	else
	{
		m_fAttackRight = 0;
	}

	if( m_fAttackRear > 0.4f )
	{
		if( flTime < m_flGasIndicatorUntil ) { r = 255; g = 0; b = 0; } else GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackRear, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 2 ) / 2;
		y = ScreenHeight / 2 + SPR_Height( m_hSprite, 2 ) * 2;
		SPR_DrawAdditive( 2, x, y, NULL );
		m_fAttackRear = Q_max( 0, m_fAttackRear - fFade );
	}
	else
	{
		m_fAttackRear = 0;
	}

	if( m_fAttackLeft > 0.4f )
	{
		if( flTime < m_flGasIndicatorUntil ) { r = 255; g = 0; b = 0; } else GetPainColor( r, g, b );
		shade = a * Q_max( m_fAttackLeft, 0.5f );
		ScaleColors( r, g, b, shade );
		SPR_Set( m_hSprite, r, g, b );

		x = ScreenWidth / 2 - SPR_Width( m_hSprite, 3 ) * 3;
		y = ScreenHeight / 2 - SPR_Height( m_hSprite, 3 ) / 2;
		SPR_DrawAdditive( 3, x, y, NULL );

		m_fAttackLeft = Q_max( 0, m_fAttackLeft - fFade );
	}
	else
	{
		m_fAttackLeft = 0;
	}

	return 1;
}

int CHudHealth::DrawDamage( float flTime )
{
	int r, g, b, a, i;
	DAMAGE_IMAGE *pdmg;

	if( !m_bitsDamage )
		return 1;

	r = gHUD.m_color.r;
	g = gHUD.m_color.g;
	b = gHUD.m_color.b;
	a = (int)( fabs( sin( flTime * 2 )) * 256.0f );

	ScaleColors( r, g, b, a );

	// Draw all the items
	for( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		if( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg = &m_dmg[i];
			SPR_Set( gHUD.GetSprite( m_HUD_dmg_bio + i ), r, g, b );
			SPR_DrawAdditive( 0, pdmg->x, pdmg->y, &gHUD.GetSpriteRect( m_HUD_dmg_bio + i ));
		}
	}

	// check for bits that should be expired
	for( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		DAMAGE_IMAGE *pdmg = &m_dmg[i];

		if( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = Q_min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if ( pdmg->fExpire <= flTime		// when the time has expired
				&& a < 40 )		// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for( int j = 0; j < NUM_DMG_TYPES; j++ )
				{
					pdmg = &m_dmg[j];
					if(( pdmg->y ) && ( pdmg->y < y ))
						pdmg->y += giDmgHeight;
				}

				m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}

	return 1;
}
 

void CHudHealth::UpdateTiles( float flTime, long bitsDamage )
{	
	DAMAGE_IMAGE *pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;
	
	for( int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if( !pdmg->fBaseline )
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if( bitsOn & giDmgFlags[i] )
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth / 8;
			const int healthSpace = s_hSoldier ? 100 + gHUD.m_iFontHeight / 2 + 8 : 0;
			pdmg->y = ScreenHeight - Q_max( giDmgHeight * 2, healthSpace + giDmgHeight );
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;
			
			// move everyone else up
			for( int j = 0; j < NUM_DMG_TYPES; j++ )
			{
				if( j == i )
					continue;

				pdmg = &m_dmg[j];
				if( pdmg->y )
					pdmg->y -= giDmgHeight;

			}

			pdmg = &m_dmg[i];
		}	
	}	

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}

