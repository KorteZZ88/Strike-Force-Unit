#include "hud.h"
#include "parsemsg.h"
#include "utils.h"
#include "r_view.h"
DECLARE_MESSAGE(m_BombMode,BombHud); DECLARE_MESSAGE(m_BombMode,ActionBar);
DECLARE_MESSAGE(m_Car,SelAmmo);
int CHudBombMode::Init(){HOOK_MESSAGE(BombHud);HOOK_MESSAGE(ActionBar);gHUD.AddHudElem(this);m_iFlags=HUD_ACTIVE|HUD_INTERMISSION;Reset();return 1;}
void CHudBombMode::Reset(){m_seconds=-1;m_restartSeconds=-1;m_red=m_blue=m_state=m_action=0;m_actionStart=m_actionDuration=0;Q_strncpy(m_team1,"Red",sizeof(m_team1));Q_strncpy(m_team2,"Blue",sizeof(m_team2));ResetRaceHudData();}
int CHudBombMode::MsgFunc_BombHud(const char*n,int s,void*p){BEGIN_READ(n,p,s);m_seconds=READ_SHORT();if(m_seconds==-32768){gHUD.m_Teamplay=3;int subtype=READ_BYTE();if(subtype==1)ReadRaceHudMessage();else if(subtype==2)ReadRaceDataMessage();else if(subtype==3)ReadRaceSpectatorCameraMessage();END_READ();return 1;}gHUD.m_Teamplay=2;ResetRaceHudData();m_red=READ_SHORT();m_blue=READ_SHORT();m_state=READ_BYTE();Q_strncpy(m_team1,READ_STRING(),sizeof(m_team1));Q_strncpy(m_team2,READ_STRING(),sizeof(m_team2));m_restartSeconds=READ_SHORT();m_iFlags|=HUD_ACTIVE;END_READ();return 1;}
int CHudBombMode::MsgFunc_ActionBar(const char*n,int s,void*p){BEGIN_READ(n,p,s);m_action=READ_BYTE();int tenths=READ_SHORT();m_actionStart=gHUD.m_flTime;m_actionDuration=tenths/10.0f;return 1;}
int CHudBombMode::Draw(float time)
{
	if(gHUD.m_Teamplay==3)
	{
		DrawRaceHud(time);
		if(m_action&&m_actionDuration>0){float f=bound(0.0f,(time-m_actionStart)/m_actionDuration,1.0f);int w=(int)(ScreenWidth*.45f),h=10,x=(ScreenWidth-w)/2,y=(int)(ScreenHeight*.68f);FillRGBA(x,y,w,h,20,20,20,190);FillRGBA(x+2,y+2,(int)((w-4)*f),h-4,50,120,255,230);if(f>=1)m_action=0;}
		return 1;
	}
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
	CVAR_REGISTER("car_debug", "0", FCVAR_ARCHIVE);
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
	if (signature == 0xCA)
		SetVehicleState((flags & 1) != 0, flags, (float)speed);
	else if (signature == 0xCB)
		SetDrivetrainState(flags - 1, (float)(unsigned short)speed, m_flEngineTorque);
	else if (signature == 0xCC)
		SetDrivetrainState(m_iGear, m_flEngineRPM, (float)(unsigned short)speed);
	else if (signature == 0xCD)
		SetConverterState((float)speed, flags / 100.0f, m_flTransmittedTorque);
	else if (signature == 0xCE)
		SetConverterState(m_flConverterSlipRPM, m_flConverterRatio, (float)(unsigned short)speed);
	return 1;
}

void CHudCar::Reset()
{
	m_bVisible = m_bEngineOn = m_bHeadlightsOn = m_bParkingBrakeOn = false;
	m_flSpeedKmh = 0.0f;
	m_iGear = 0;
	m_flEngineRPM = 0.0f;
	m_flEngineTorque = 0.0f;
	m_flConverterSlipRPM = 0.0f;
	m_flConverterRatio = 1.0f;
	m_flTransmittedTorque = 0.0f;
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

void CHudCar::SetDrivetrainState(int gear, float engineRPM, float engineTorque)
{
	m_iGear = gear;
	m_flEngineRPM = Q_max(0.0f, engineRPM);
	m_flEngineTorque = Q_max(0.0f, engineTorque);
}

void CHudCar::SetConverterState(float slipRPM, float ratio, float transmittedTorque)
{
	m_flConverterSlipRPM = slipRPM;
	m_flConverterRatio = Q_max(1.0f, ratio);
	m_flTransmittedTorque = Q_max(0.0f, transmittedTorque);
}

static int CarLargeNumberWidth(int value, int digitWidth)
{
	int digits = 1;
	for (value = abs(value); value >= 10; value /= 10) ++digits;
	return digits * digitWidth;
}

static int DrawCarLargeNumber(int x, int y, int value, int r, int g, int b)
{
	char digits[16];
	Q_snprintf(digits, sizeof(digits), "%d", Q_max(0, value));
	for (const char *digit = digits; *digit; ++digit)
	{
		const int index = bound(0, *digit - '0', 9);
		SPR_Set(gHUD.GetSprite(gHUD.m_HUD_number_0 + index), r, g, b);
		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(gHUD.m_HUD_number_0 + index));
		x += gHUD.GetSpriteRect(gHUD.m_HUD_number_0 + index).right -
			gHUD.GetSpriteRect(gHUD.m_HUD_number_0 + index).left;
	}
	return x;
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

	// Native health/ammo digit sprites, grouped along the bottom edge.
	const int digitWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right -
		gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
	const int speedY = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	const int labelY = speedY - line;
	const int speedValue = Q_min(999, (int)(m_flSpeedKmh + 0.5f));
	const int rpmValue = Q_min(99999, (int)(m_flEngineRPM + 0.5f));
	const int torqueValue = Q_min(99999, (int)(m_flEngineTorque + 0.5f));
	const int gap = Q_max(12, digitWidth / 2);
	const int totalWidth = CarLargeNumberWidth(speedValue, digitWidth) + digitWidth +
		CarLargeNumberWidth(rpmValue, digitWidth) + CarLargeNumberWidth(torqueValue, digitWidth) + gap * 3;
	int valueX = (ScreenWidth - totalWidth) / 2;

	gHUD.DrawHudString(valueX, labelY, ScreenWidth, "SPEED KM/H", offR, offG, offB);
	valueX = DrawCarLargeNumber(valueX, speedY, speedValue, offR, offG, offB) + gap;
	const char *gearLabel = m_iGear < 0 ? "GEAR R" : (m_iGear == 0 ? "GEAR N" : "GEAR");
	gHUD.DrawHudString(valueX, labelY, ScreenWidth, gearLabel, offR, offG, offB);
	valueX = DrawCarLargeNumber(valueX, speedY, abs(m_iGear), offR, offG, offB) + gap;
	gHUD.DrawHudString(valueX, labelY, ScreenWidth, "ENGINE RPM", offR, offG, offB);
	valueX = DrawCarLargeNumber(valueX, speedY, rpmValue, offR, offG, offB) + gap;
	gHUD.DrawHudString(valueX, labelY, ScreenWidth, "ENGINE TORQUE", offR, offG, offB);
	DrawCarLargeNumber(valueX, speedY, torqueValue, offR, offG, offB);
	if (CVAR_GET_FLOAT("car_debug") >= 2.0f)
	{
		Q_snprintf(text, sizeof(text), "CONVERTER SLIP %.0f RPM   RATIO %.2f   TRANSMITTED TORQUE %.0f",
			m_flConverterSlipRPM, m_flConverterRatio, m_flTransmittedTorque);
		gHUD.DrawHudString((ScreenWidth - ConsoleStringLen(text)) / 2,
			labelY - line, ScreenWidth, text, offR, offG, offB);
	}

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
