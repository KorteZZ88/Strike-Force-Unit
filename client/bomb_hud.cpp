#include "hud.h"
#include "parsemsg.h"
#include "utils.h"
#include "r_view.h"
DECLARE_MESSAGE(m_BombMode,BombHud); DECLARE_MESSAGE(m_BombMode,ActionBar);
DECLARE_MESSAGE(m_Car,SelAmmo);
int CHudBombMode::Init(){HOOK_MESSAGE(BombHud);HOOK_MESSAGE(ActionBar);gHUD.AddHudElem(this);m_iFlags=HUD_ACTIVE|HUD_INTERMISSION;Reset();return 1;}
void CHudBombMode::Reset(){m_seconds=-1;m_restartSeconds=-1;m_red=m_blue=m_state=m_action=0;m_actionStart=m_actionDuration=0;Q_strncpy(m_team1,"Red",sizeof(m_team1));Q_strncpy(m_team2,"Blue",sizeof(m_team2));}
int CHudBombMode::MsgFunc_BombHud(const char*n,int s,void*p){BEGIN_READ(n,p,s);m_seconds=READ_SHORT();m_red=READ_SHORT();m_blue=READ_SHORT();m_state=READ_BYTE();Q_strncpy(m_team1,READ_STRING(),sizeof(m_team1));Q_strncpy(m_team2,READ_STRING(),sizeof(m_team2));m_restartSeconds=READ_SHORT();m_iFlags|=HUD_ACTIVE;return 1;}
int CHudBombMode::MsgFunc_ActionBar(const char*n,int s,void*p){BEGIN_READ(n,p,s);m_action=READ_BYTE();int tenths=READ_SHORT();m_actionStart=gHUD.m_flTime;m_actionDuration=tenths/10.0f;return 1;}
int CHudBombMode::Draw(float time)
{
	if(gHUD.m_iIntermission)
	{
		char result[96];
		int r=255,g=255,b=255;
		if(m_red==m_blue) Q_strncpy(result,"The match is a draw!",sizeof(result));
		else if(m_red>m_blue){Q_snprintf(result,sizeof(result),"%s won the match!",m_team1);r=255;g=60;b=60;}
		else{Q_snprintf(result,sizeof(result),"%s won the match!",m_team2);r=70;g=130;b=255;}
		const int x=(ScreenWidth-ConsoleStringLen(result))/2,y=(int)(ScreenHeight*.38f);
		// A heavy outline makes the fixed-size engine font substantially more
		// prominent during intermission while keeping it readable on any HUD scale.
		for(int oy=-2;oy<=2;oy++)for(int ox=-2;ox<=2;ox++)if(ox||oy)gHUD.DrawHudString(x+ox,y+oy,ScreenWidth,result,0,0,0);
		gHUD.DrawHudString(x,y,ScreenWidth,result,r,g,b);
		gHUD.DrawHudString(x+1,y,ScreenWidth,result,r,g,b);
		return 1;
	}
	char redScore[64],blueScore[64];Q_snprintf(redScore,sizeof(redScore),"%s %d",m_team1,m_red);Q_snprintf(blueScore,sizeof(blueScore),"%d %s",m_blue,m_team2);int center=ScreenWidth/2;gHUD.DrawHudString(center-58-ConsoleStringLen(redScore),18,ScreenWidth,redScore,255,60,60);gHUD.DrawHudString(center+58,18,ScreenWidth,blueScore,70,130,255);
	if(m_seconds>=0){char timer[16];Q_snprintf(timer,sizeof(timer),"%d:%02d",m_seconds/60,m_seconds%60);gHUD.DrawHudString(center-ConsoleStringLen(timer)/2,18,ScreenWidth,timer,m_state?255:220,m_state?180:220,m_state?0:220);}
	if(m_restartSeconds>=0){char restart[96];Q_snprintf(restart,sizeof(restart),"Round restarting in %d seconds",m_restartSeconds);gHUD.DrawHudString(center-ConsoleStringLen(restart)/2,(int)(ScreenHeight*.27f),ScreenWidth,restart,255,255,255);}
	if(m_action&&m_actionDuration>0){float f=bound(0.0f,(time-m_actionStart)/m_actionDuration,1.0f);int w=(int)(ScreenWidth*.45f),h=10,x=(ScreenWidth-w)/2,y=(int)(ScreenHeight*.68f);FillRGBA(x,y,w,h,20,20,20,190);FillRGBA(x+2,y+2,(int)((w-4)*f),h-4,m_action==1?220:50,m_action==1?80:120,m_action==1?40:255,230);if(f>=1)m_action=0;}
	return 1;
}

int CHudCar::Init()
{
	HOOK_MESSAGE(SelAmmo);
	// Drawn explicitly by CHud::Redraw. The vehicle overlay must not disappear
	// when the generic HUD list is rebuilt or weapon HUD masks are changed.
	m_iFlags = HUD_ACTIVE;
	Reset();
	return 1;
}

int CHudCar::MsgFunc_SelAmmo(const char *name, int size, void *buffer)
{
	BEGIN_READ(name, buffer, size);
	const int flags = READ_BYTE();
	const short speed = (short)READ_SHORT();
	const int signature = READ_BYTE();
	END_READ();
	if (signature != 0xCA) return 1;
	SetVehicleState((flags & 1) != 0, flags, (float)speed);
	return 1;
}

void CHudCar::Reset()
{
	m_bVisible = m_bEngineOn = m_bHeadlightsOn = m_bParkingBrakeOn = false;
	m_flSpeedKmh = 0.0f;
	m_flHintsUntil = 0.0f;
}

void CHudCar::SetVehicleState(bool visible, int flags, float speed)
{
	// The tagged view entity itself is authoritative for visibility. Requiring a
	// second networked bit made the whole HUD disappear on an initial snapshot.
	if (visible && !m_bVisible)
		m_flHintsUntil = gHUD.m_flTime + 10.0f;
	if (!visible && m_bVisible)
		V_ResetCarThirdPerson();
	m_bVisible = visible;
	m_bEngineOn = (flags & 2) != 0;
	m_bHeadlightsOn = (flags & 4) != 0;
	m_bParkingBrakeOn = (flags & 8) != 0;
	// GoldSrc/Xash world units are inches. Convert units/second to km/h.
	m_flSpeedKmh = fabs(speed) * 0.09144f;
	if (m_flSpeedKmh >= 1.0f)
		m_flHintsUntil = 0.0f;
}

int CHudCar::Draw(float time)
{
	if (!m_bVisible || gHUD.m_iIntermission) return 1;
	const int x = 24;
	const int line = Q_max(12, gHUD.m_iFontHeight + 2);
	// Keep the three status lines immediately above the normal health display.
	const int healthY = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int y = healthY - line * 3 - 6;
	char text[128];
	const int offR = 255, offG = 210, offB = 60;
	const int onR = 80, onG = 255, onB = 80;
	Q_snprintf(text, sizeof(text), "Engine: %s", m_bEngineOn ? "On" : "Off");
	gHUD.DrawHudString(x, y, ScreenWidth, text, m_bEngineOn ? onR : offR, m_bEngineOn ? onG : offG, m_bEngineOn ? onB : offB); y += line;
	Q_snprintf(text, sizeof(text), "Headlights: %s", m_bHeadlightsOn ? "On" : "Off");
	gHUD.DrawHudString(x, y, ScreenWidth, text, m_bHeadlightsOn ? onR : offR, m_bHeadlightsOn ? onG : offG, m_bHeadlightsOn ? onB : offB); y += line;
	Q_snprintf(text, sizeof(text), "Parking Brake: %s", m_bParkingBrakeOn ? "On" : "Off");
	gHUD.DrawHudString(x, y, ScreenWidth, text, m_bParkingBrakeOn ? 255 : offR, m_bParkingBrakeOn ? 70 : offG, m_bParkingBrakeOn ? 70 : offB);

	// Native health/ammo digit sprites, centred along the bottom edge.
	const int digitWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right -
		gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
	const int speedY = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	const int speedX = ScreenWidth / 2 - (digitWidth * 3) / 2;
	const int speedEnd = gHUD.DrawHudNumber(speedX, speedY,
		DHN_3DIGITS | DHN_DRAWZERO, Q_min(999, (int)(m_flSpeedKmh + 0.5f)), offR, offG, offB);
	gHUD.DrawHudString(speedEnd + 6, speedY, ScreenWidth, "KM/H", offR, offG, offB);

	if (m_flHintsUntil > time)
	{
		const char *hints[] = {
			"G - Engine        WASD - Movement",
			"SPACE - Handbrake        F - Headlights",
			"LMB - Horn        X - Parking brake",
			"E - Exit"
		};
		int hintY = speedY - line * 5;
		for (const char *hint : hints)
		{
			const int hintX = (ScreenWidth - ConsoleStringLen(hint)) / 2;
			gHUD.DrawHudString(hintX, hintY, ScreenWidth, hint, offR, offG, offB);
			hintY += line;
		}
	}
	return 1;
}
