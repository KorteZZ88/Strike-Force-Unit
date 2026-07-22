#include "hud.h"
#include "parsemsg.h"
#include "utils.h"
DECLARE_MESSAGE(m_BombMode,BombHud); DECLARE_MESSAGE(m_BombMode,ActionBar);
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
