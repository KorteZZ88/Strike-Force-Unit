#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "game.h"
#include "bomb_gamerules.h"
#include "user_messages.h"
#include "entities/bomb_objective.h"
#include "entities/func_break.h"
#include "entities/func_door.h"
#include "entities/func_button.h"
#include "entities/func_platform.h"
#include "../../game_shared/pm_shared.h"
#include "../../game_shared/weapons/glock.h"
#include "../../game_shared/weapons/beretta.h"
#include "../../game_shared/weapons/p229.h"
#include "../../game_shared/weapons/fiveseven.h"
#include "../../game_shared/weapons/glock18.h"
#include "../../game_shared/weapons/shotgun.h"
#include "../../game_shared/weapons/m3.h"
#include "../../game_shared/weapons/mp5.h"
#include "../../game_shared/weapons/mp5a3.h"
#include "../../game_shared/weapons/mp5sd.h"
#include "../../game_shared/weapons/mac10.h"
#include "../../game_shared/weapons/tmp.h"
#include "../../game_shared/weapons/ump.h"
#include "../../game_shared/weapons/p90.h"
#include "../../game_shared/weapons/bizon.h"
#include "../../game_shared/weapons/m4.h"
#include "../../game_shared/weapons/m24.h"
#include "../../game_shared/weapons/m72.h"
#include "../../game_shared/weapons/ak47.h"
#include "../../game_shared/weapons/galil.h"
#include "../../game_shared/weapons/famas.h"
#include "../../game_shared/weapons/sg552.h"
#include "../../game_shared/weapons/aug.h"
#include "../../game_shared/weapons/m60.h"
#include "../../game_shared/weapons/python.h"
#include "../../game_shared/weapons/ragingbull.h"
#include "../../game_shared/weapons/deagle.h"
#include "../../game_shared/weapons/usp.h"
#include "../../game_shared/weapons/colt1911.h"
#include "../../game_shared/weapons/crowbar.h"
#include "../../game_shared/weapons/handgrenade.h"
#include "../../game_shared/weapons/flashbang.h"
#include "../../game_shared/weapons/gasgrenade.h"
extern void respawn(CBaseEntity*,BOOL);
extern BOOL IsSpawnPointValid(CBaseEntity*,CBaseEntity*);
static const char *RED="red", *BLUE="blue", *SPEC="spectator";
static bool g_bBombNamesInitialized=false;
static char g_szNextTeam1Name[32]="Red",g_szNextTeam2Name[32]="Blue";
static int m_spectatorTarget[65]={};
static float m_nextSpectatorHud[65]={};
static float BombRoundSeconds(){return Q_max(6.0f,bomb_roundtime.value*60.0f);}
static int MoneyLimit(){return bomb_moneylimit.value<=0?INT_MAX:Q_max(1,(int)bomb_moneylimit.value);}
static void HudNotice(CBasePlayer*p,const char*text){if(!p)return;MESSAGE_BEGIN(MSG_ONE,gmsgPickupHint,NULL,p->pev);WRITE_STRING(text?text:"");MESSAGE_END();}
static void HudNoticeAll(const char*text){for(int i=1;i<=gpGlobals->maxClients;i++)HudNotice((CBasePlayer*)UTIL_PlayerByIndex(i),text);}
static bool HasWeaponId(CBasePlayer*p,int weaponId){if(!p)return false;for(int slot=0;slot<MAX_ITEM_TYPES;slot++)for(CBasePlayerItem*item=p->m_rgpPlayerItems[slot];item;item=item->m_pNext)if(item->iWeaponID()==weaponId)return true;return false;}

class CDroppedMoney:public CBaseEntity
{
public:
	void Spawn()override{Precache();SET_MODEL(edict(),"models/money.mdl");pev->movetype=MOVETYPE_TOSS;pev->solid=SOLID_TRIGGER;UTIL_SetSize(pev,Vector(-8,-8,-8),Vector(8,8,8));SetTouch(&CDroppedMoney::MoneyTouch);}
	void Precache()override{PRECACHE_MODEL("models/money.mdl");PRECACHE_SOUND("items/gunpickup2.wav");}
	void MoneyTouch(CBaseEntity*other){CBasePlayer*p=dynamic_cast<CBasePlayer*>(other);if(!p||!p->IsAlive()||(pev->owner==p->edict()&&gpGlobals->time<pev->fuser1))return;CBombGameRules*rules=dynamic_cast<CBombGameRules*>(g_pGameRules);if(rules&&pev->iuser1>0&&rules->PickupDroppedMoney(p,pev->iuser1)){EMIT_SOUND(p->edict(),CHAN_ITEM,"items/gunpickup2.wav",1.0f,ATTN_NORM);UTIL_Remove(this);}}
};
LINK_ENTITY_TO_CLASS(dropped_money,CDroppedMoney);
CBombGameRules::CBombGameRules() : m_state(ACTIVE),m_roundEnd(gpGlobals->time+BombRoundSeconds()),m_nextRound(0),m_redWins(0),m_blueWins(0),m_lastSecond(-1),m_roundStartEquipment(std::make_unique<EquipmentSnapshot[]>(65)),m_transitionEquipment(std::make_unique<EquipmentSnapshot[]>(65)),m_roundStartGroundWeapons(std::make_unique<GroundWeaponSnapshot[]>(64)),m_transitionGroundWeapons(std::make_unique<GroundWeaponSnapshot[]>(64)) { memset(m_spectatorTarget,0,sizeof(m_spectatorTarget));memset(m_nextSpectatorHud,0,sizeof(m_nextSpectatorHud));SERVER_COMMAND("exec game.cfg\n");COMMAND_EXECUTE();m_roundEnd=gpGlobals->time+Q_max(6.0f,CVAR_GET_FLOAT("mp_roundtime")*60.0f);m_bomb=NULL;m_c4Timer=Q_max(1.0f,CVAR_GET_FLOAT("mp_c4timer"));if(!g_bBombNamesInitialized){Q_strncpy(g_szNextTeam1Name,CVAR_GET_STRING("sv_team1name"),sizeof(g_szNextTeam1Name));Q_strncpy(g_szNextTeam2Name,CVAR_GET_STRING("sv_team2name"),sizeof(g_szNextTeam2Name));g_bBombNamesInitialized=true;}Q_strncpy(m_team1Name,g_szNextTeam1Name,sizeof(m_team1Name));Q_strncpy(m_team2Name,g_szNextTeam2Name,sizeof(m_team2Name));if(!m_team1Name[0])Q_strncpy(m_team1Name,"Red",sizeof(m_team1Name));if(!m_team2Name[0])Q_strncpy(m_team2Name,"Blue",sizeof(m_team2Name));CVAR_SET_STRING("sv_team1name",m_team1Name);CVAR_SET_STRING("sv_team2name",m_team2Name);ALERT(at_console,"Bomb config: roundlimit %g, winlimit %g\n",CVAR_GET_FLOAT("mp_roundlimit"),CVAR_GET_FLOAT("mp_winlimit")); }
int CBombGameRules::GetTeamIndex(const char*n){if(!n)return -1;if(!Q_stricmp(n,RED))return 0;if(!Q_stricmp(n,BLUE))return 1;return -1;}
const char *CBombGameRules::GetIndexedTeamName(int i){return i==0?RED:i==1?BLUE:"";}
BOOL CBombGameRules::IsValidTeam(const char*n){return GetTeamIndex(n)>=0;}
const char *CBombGameRules::SetDefaultPlayerTeam(CBasePlayer*p)
{
	// Spawn() calls this on every respawn. Keep an explicit Red/Blue choice;
	// clearing it here made GetPlayerSpawnSpot() fall through to Blue.
	if(IsValidTeam(p->m_szTeamName))return p->m_szTeamName;
	p->m_szTeamName[0]=0;
	return p->m_szTeamName;
}
void CBombGameRules::ChangePlayerTeam(CBasePlayer*p,const char*n,BOOL,BOOL)
{
	Q_strncpy(p->m_szTeamName,n,TEAM_NAME_LENGTH); int idx=p->entindex(); const char *model=!Q_stricmp(n,RED)?"gordon":(!Q_stricmp(n,BLUE)?"zombie":"");
	g_engfuncs.pfnSetClientKeyValue(idx,g_engfuncs.pfnGetInfoKeyBuffer(p->edict()),"model",model); g_engfuncs.pfnSetClientKeyValue(idx,g_engfuncs.pfnGetInfoKeyBuffer(p->edict()),"team",n);
	MESSAGE_BEGIN(MSG_ALL,gmsgTeamInfo);WRITE_BYTE(idx);WRITE_STRING(n);MESSAGE_END();
}
void CBombGameRules::ShowTeamMenu(CBasePlayer*p){char text[256];bool exit=m_hasTeamChoice[p->entindex()]||m_state==FINISHED;Q_snprintf(text,sizeof(text),"Choose team\n\n1. %s\n2. %s\n3. Random\n\n\n6. Spectator%s",m_team1Name,m_team2Name,exit?"\n\n0. Exit":"");MESSAGE_BEGIN(MSG_ONE,gmsgShowMenu,NULL,p->pev);WRITE_SHORT((1<<0)|(1<<1)|(1<<2)|(1<<5)|(exit?(1<<9):0));WRITE_CHAR(-1);WRITE_BYTE(FALSE);WRITE_STRING(text);MESSAGE_END();}
void CBombGameRules::CloseTeamMenu(CBasePlayer*p){MESSAGE_BEGIN(MSG_ONE,gmsgShowMenu,NULL,p->pev);WRITE_SHORT(0);WRITE_CHAR(0);WRITE_BYTE(FALSE);WRITE_STRING("");MESSAGE_END();}
void CBombGameRules::InitHUD(CBasePlayer*p){CHalfLifeMultiplay::InitHUD(p);MESSAGE_BEGIN(MSG_ONE,gmsgTeamNames,NULL,p->edict());WRITE_BYTE(2);WRITE_STRING(m_team1Name);WRITE_STRING(m_team2Name);MESSAGE_END();for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*other=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!other)continue;MESSAGE_BEGIN(MSG_ONE,gmsgTeamInfo,NULL,p->edict());WRITE_BYTE(i);WRITE_STRING(other->TeamID());MESSAGE_END();const bool hideBomb=!Q_stricmp(p->TeamID(),BLUE);MESSAGE_BEGIN(MSG_ONE,gmsgScoreInfo,NULL,p->edict());WRITE_BYTE(i);WRITE_SHORT(0);WRITE_SHORT(other->m_iDeaths);WRITE_SHORT(other->IsAlive()?(other->HasNamedPlayerItem("weapon_bomb")&&!hideBomb?2:1):0);WRITE_SHORT(GetTeamIndex(other->m_szTeamName)+1);MESSAGE_END();}ShowTeamMenu(p);if(!m_hasTeamChoice[p->entindex()])StartTeamMenuCamera(p);SendHud();}
void CBombGameRules::UpdateGameMode(CBasePlayer*p){MESSAGE_BEGIN(MSG_ONE,gmsgGameMode,NULL,p->edict());WRITE_BYTE(2);MESSAGE_END();}
BOOL CBombGameRules::ClientCommand(CBasePlayer*p,const char*cmd)
{
	if(FStrEq(cmd,"buymenu")){ShowBuyMenu(p);return TRUE;}
	if(FStrEq(cmd,"buyequip")){ShowBuyMenu(p,8);return TRUE;}
	if(FStrEq(cmd,"primammo")){BuyAmmo(p,true,false);return TRUE;}
	if(FStrEq(cmd,"secammo")){BuyAmmo(p,false,false);return TRUE;}
	if(FStrEq(cmd,"sellweapon")){SellWeapon(p);return TRUE;}
	if(FStrEq(cmd,"dropmoney")){DropMoney(p);return TRUE;}
	if(FStrEq(cmd,"dropweapon"))
	{
		if(!p||!p->IsAlive())return TRUE;
		bool bomb=p->m_pActiveItem&&FClassnameIs(p->m_pActiveItem->pev,"weapon_bomb");
		p->DropPlayerItem((char*)"");
		if(bomb&&!p->HasNamedPlayerItem("weapon_bomb")){SendScoreStatus(p,1);TeamNotice(RED,"Bomb lost");}
		return TRUE;
	}
	if(FStrEq(cmd,"menuselect")&&m_buyMenuActive[p->entindex()]){if(CMD_ARGC()>1)SelectBuyMenu(p,atoi(CMD_ARGV(1)));return TRUE;}
	if(FStrEq(cmd,"menuselect")){if(CMD_ARGC()>1)SelectTeam(p,atoi(CMD_ARGV(1)));return TRUE;} if(FStrEq(cmd,"chooseteam")||FStrEq(cmd,"changeteam")){ShowTeamMenu(p);return TRUE;} return FALSE;
}
void CBombGameRules::ClientUserInfoChanged(CBasePlayer*p,char*)
{
	if(!p)return;
	const char *model=!Q_stricmp(p->TeamID(),RED)?"gordon":(!Q_stricmp(p->TeamID(),BLUE)?"zombie":"");
	if(!model[0])return;
	int idx=p->entindex(); char *info=g_engfuncs.pfnGetInfoKeyBuffer(p->edict());
	g_engfuncs.pfnSetClientKeyValue(idx,info,"model",model);
	g_engfuncs.pfnSetClientKeyValue(idx,info,"team",p->TeamID());
}
void CBombGameRules::SelectTeam(CBasePlayer*p,int s)
{
	if(s==10||s==0){CloseTeamMenu(p);return;} if(s==3)s=RANDOM_LONG(1,2); if(s!=1&&s!=2&&s!=6)return;
	int idx=p->entindex();StopTeamMenuCamera(p);CloseTeamMenu(p);
	// A spectator who commits to a combat team is listed there immediately, but
	// remains a dead observer until StartRound consumes the pending choice.
	if(m_hasTeamChoice[idx]&&!Q_stricmp(p->TeamID(),SPEC)&&s!=6)
	{
		m_pendingTeam[idx]=s;
		SET_VIEW(p->edict(),p->edict());p->pev->iuser1=p->pev->iuser2=0;m_spectatorTarget[idx]=0;
		MESSAGE_BEGIN(MSG_ONE,gmsgSpecTarget,NULL,p->pev);WRITE_BYTE(0);WRITE_BYTE(0);MESSAGE_END();
		ChangePlayerTeam(p,s==1?RED:BLUE,FALSE,FALSE);
		p->RemoveAllItems(TRUE);
		p->pev->health=0;
		p->pev->deadflag=DEAD_DEAD;
		ClearBits(p->pev->flags,FL_SPECTATOR);
		p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);
		SendScoreStatus(p,0);
		return;
	}
	if(m_state==ACTIVE&&m_hasTeamChoice[idx]){m_pendingTeam[idx]=s;return;}
	m_pendingTeam[idx]=0;
	m_hasTeamChoice[idx]=true;
	ApplyTeamChoice(p,s,m_state==ACTIVE);
}
void CBombGameRules::ApplyTeamChoice(CBasePlayer*p,int s,bool spawnNow)
{
	if(s==6){ChangePlayerTeam(p,SPEC,FALSE,FALSE);SendScoreStatus(p,0);p->RemoveAllItems(TRUE);p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);p->pev->movetype=MOVETYPE_NOCLIP;p->pev->flags|=FL_SPECTATOR;m_spectatorTarget[p->entindex()]=0;m_nextSpectatorHud[p->entindex()]=0;return;}
	int idx=p->entindex();bool changedCombatTeam=m_moneyTeam[idx]!=0&&m_moneyTeam[idx]!=s&&!m_executingForcedRestart;m_moneyTeam[idx]=s;ChangePlayerTeam(p,s==1?RED:BLUE,FALSE,FALSE);if(changedCombatTeam){EnsureMoney(p);int startMoney=bound(0,(int)CVAR_GET_FLOAT("mp_startmoney"),MoneyLimit());AddMoney(p,startMoney-m_money[idx]);}SendScoreStatus(p,p->IsAlive()?1:0);if(spawnNow){ClearBits(p->pev->flags,FL_SPECTATOR);ClearBits(p->m_afPhysicsFlags,PFLAG_OBSERVER);respawn(p,FALSE);if(s==1&&!m_bomb)GiveCarrier();}
}
void CBombGameRules::SendScoreStatus(CBasePlayer*p,int status){if(!p)return;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*viewer=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!viewer)continue;const int visibleStatus=(status==2&&!Q_stricmp(viewer->TeamID(),BLUE))?1:status;MESSAGE_BEGIN(MSG_ONE,gmsgScoreInfo,NULL,viewer->edict());WRITE_BYTE(p->entindex());WRITE_SHORT(0);WRITE_SHORT(p->m_iDeaths);WRITE_SHORT(visibleStatus);WRITE_SHORT(GetTeamIndex(p->m_szTeamName)+1);MESSAGE_END();}}
void CBombGameRules::PlayerSpawn(CBasePlayer*p){EnsureMoney(p);if(!IsValidTeam(p->TeamID())){p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);return;}CHalfLifeMultiplay::PlayerSpawn(p);if(!p->HasNamedPlayerItem("weapon_crowbar"))p->GiveNamedItem("weapon_crowbar");if(p->m_rgpPlayerItems[2]==NULL){const bool blue=!Q_stricmp(p->TeamID(),BLUE);p->GiveNamedItem(blue?"weapon_beretta":"weapon_glock18");m_weaponPurchased[p->entindex()][blue?WEAPON_BERETTA:WEAPON_GLOCK18]=false;}SetKnifeAsLastItem(p);SendScoreStatus(p,1);}
BOOL CBombGameRules::FPlayerCanRespawn(CBasePlayer*){return FALSE;}
BOOL CBombGameRules::FPlayerCanTakeDamage(CBasePlayer*p,CBaseEntity*a){if(a&&a->IsPlayer()&&a!=p&&!Q_stricmp(p->TeamID(),a->TeamID()))return FALSE;return TRUE;}
BOOL CBombGameRules::CanHavePlayerItem(CBasePlayer*p,CBasePlayerItem*w){if(w&&FClassnameIs(w->pev,"weapon_bomb")&&Q_stricmp(p->TeamID(),RED))return FALSE;return CHalfLifeMultiplay::CanHavePlayerItem(p,w);}
void CBombGameRules::PlayerGotWeapon(CBasePlayer*p,CBasePlayerItem*w){CHalfLifeMultiplay::PlayerGotWeapon(p,w);if(w&&FClassnameIs(w->pev,"weapon_bomb")){SendScoreStatus(p,2);if(!m_givingCarrier)TeamNotice(RED,"Bomb picked up");}}
edict_t *CBombGameRules::GetPlayerSpawnSpot(CBasePlayer*p)
{
	const bool isRed=!Q_stricmp(p->TeamID(),RED);
	const char *cn=isRed?"info_player_red":"info_player_blue"; CBaseEntity *spots[64];int count=0;CBaseEntity *it=NULL;while(count<64&&(it=UTIL_FindEntityByClassname(it,cn))!=NULL)spots[count++]=it;
	if(!count){cn=isRed?"info_player_deathmatch":"info_player_start";it=NULL;while(count<64&&(it=UTIL_FindEntityByClassname(it,cn))!=NULL)spots[count++]=it;}
	CBaseEntity *spot=NULL;
	if(count){const int first=RANDOM_LONG(0,count-1);for(int offset=0;offset<count;offset++){CBaseEntity*candidate=spots[(first+offset)%count];if(IsSpawnPointValid(p,candidate)){spot=candidate;break;}}if(!spot)spot=spots[first];}
	if(!spot)return CHalfLifeMultiplay::GetPlayerSpawnSpot(p);
	p->SetAbsOrigin(spot->GetAbsOrigin()+Vector(0,0,1));p->SetAbsAngles(spot->GetAbsAngles());p->pev->v_angle=g_vecZero;p->pev->fixangle=TRUE;
	p->SetAbsVelocity(g_vecZero);p->pev->basevelocity=g_vecZero;p->pev->avelocity=g_vecZero;p->pev->movedir=g_vecZero;p->m_flFallVelocity=0;ClearBits(p->pev->flags,FL_BASEVELOCITY);
	UTIL_DropToFloor(p);
	return spot->edict();
}
void CBombGameRules::PlayerKilled(CBasePlayer*p,entvars_t*k,entvars_t*i){bool had=p->HasNamedPlayerItem("weapon_bomb");CBasePlayer*killer=k&&k!=p->pev&&FBitSet(k->flags,FL_CLIENT)?static_cast<CBasePlayer*>(CBaseEntity::Instance(ENT(k))):NULL;if(killer&&Q_stricmp(killer->TeamID(),p->TeamID())){int reward=300;CBasePlayerWeapon*w=NULL;if(i==k)w=dynamic_cast<CBasePlayerWeapon*>(killer->m_pActiveItem);if(w){switch(w->iWeaponID()){case WEAPON_MP5A3:reward=600;break;case WEAPON_M3:reward=900;break;case WEAPON_CROWBAR:reward=1500;break;}}AddMoney(killer,reward);}int idx=p->entindex();m_diedThisRound[idx]=true;m_hasDefuseKit[idx]=false;m_hasNightVision[idx]=false;CHalfLifeMultiplay::PlayerKilled(p,k,i);p->pev->frags=0;if(had){p->DropPlayerItem((char*)"weapon_bomb");TeamNotice(RED,"Bomb lost");}CheckElimination();}
void CBombGameRules::ClientDisconnected(edict_t*e){CBasePlayer*p=(CBasePlayer*)CBaseEntity::Instance(e);int idx=p?p->entindex():0;if(p&&p->HasNamedPlayerItem("weapon_bomb")){p->DropPlayerItem((char*)"weapon_bomb");TeamNotice(RED,"Bomb lost");}CHalfLifeMultiplay::ClientDisconnected(e);if(idx>0&&idx<65){m_pendingTeam[idx]=0;m_hasTeamChoice[idx]=false;m_teamMenuCameraActive[idx]=false;m_teamMenuCameraIndex[idx]=0;m_nextTeamMenuCamera[idx]=0;m_plantHintShown[idx]=false;m_moneyInitialized[idx]=false;m_moneyTeam[idx]=0;m_buyMenuActive[idx]=false;m_hasDefuseKit[idx]=false;m_hasNightVision[idx]=false;m_diedThisRound[idx]=false;memset(m_weaponFired[idx],0,sizeof(m_weaponFired[idx]));}CheckElimination();}
BOOL CBombGameRules::CanPlantBomb(CBasePlayer*p){if(m_state!=ACTIVE||m_bomb||Q_stricmp(p->TeamID(),RED))return FALSE;CBaseEntity*t=NULL;while((t=UTIL_FindEntityByClassname(t,"func_bomb_target"))!=NULL)if(p->Intersects(t))return TRUE;return FALSE;}
void CBombGameRules::PlantBomb(CBasePlayer*p){if(!CanPlantBomb(p))return;m_bomb=CBaseEntity::Create("planted_bomb",p->GetAbsOrigin(),Vector(0,p->pev->angles.y,0),p->edict());AddMoney(p,800);EMIT_SOUND(p->edict(),CHAN_ITEM,"weapons/Bomb/c4_plant.wav",1.0f,ATTN_NORM);EMIT_SOUND_DYN(p->edict(),CHAN_VOICE,"radio/bombpl.wav",1.0f,ATTN_NONE,0,PITCH_NORM);CBasePlayerItem*bomb=p->m_pActiveItem;if(bomb&&FClassnameIs(bomb->pev,"weapon_bomb")){p->SelectBestCombatWeapon(bomb);p->RemoveWeapon(bomb->iWeaponID());bomb->DestroyItem();}SendScoreStatus(p,1);p->pev->maxspeed=0;HudNoticeAll("Bomb planted");SendHud();}
void CBombGameRules::BombDefused(CBasePlayer*p){m_bomb=NULL;if(p)AddMoney(p,500);HudNoticeAll("Bomb defused");AwardRoundMoney(false,3250);EndRound(false,"Blue wins");}
void CBombGameRules::BombExploded(CBaseEntity*){m_bomb=NULL;AwardRoundMoney(true,3500);EndRound(true,"Red wins");}
void CBombGameRules::TargetActivated(const char*n){if(m_state!=ACTIVE||!n)return;if(!Q_stricmp(n,RED)){AwardRoundMoney(true);EndRound(true,UTIL_VarArgs("%s wins",m_team1Name));}else if(!Q_stricmp(n,BLUE)){AwardRoundMoney(false);EndRound(false,UTIL_VarArgs("%s wins",m_team2Name));}}
void CBombGameRules::RestartRoundIn(float seconds){m_forcedRestartAt=gpGlobals->time+Q_max(0.0f,seconds);m_lastForcedRestartSecond=-1;}

void CBombGameRules::EnsureMoney(CBasePlayer*p)
{
	if(!p)return;int i=p->entindex();if(i<1||i>=65)return;if(m_moneyInitialized[i]){m_money[i]=Q_min(m_money[i],MoneyLimit());return;}
	m_moneyInitialized[i]=true;m_money[i]=bound(0,(int)CVAR_GET_FLOAT("mp_startmoney"),MoneyLimit());
}
void CBombGameRules::AddMoney(CBasePlayer*p,int amount)
{
	if(!p)return;EnsureMoney(p);int i=p->entindex(),oldMoney=m_money[i];long long updated=(long long)m_money[i]+amount;m_money[i]=(int)Q_min((long long)MoneyLimit(),Q_max(0LL,updated));amount=m_money[i]-oldMoney;
	if(amount){MESSAGE_BEGIN(MSG_ONE,gmsgMoneyDelta,NULL,p->pev);WRITE_LONG(amount);MESSAGE_END();}
	for(int n=1;n<=gpGlobals->maxClients;n++){CBasePlayer*r=(CBasePlayer*)UTIL_PlayerByIndex(n);if(r)SendMoneyTo(r);}
}
bool CBombGameRules::PickupDroppedMoney(CBasePlayer*p,int amount)
{
	if(!p)return false;EnsureMoney(p);int before=m_money[p->entindex()];AddMoney(p,amount);return m_money[p->entindex()]>before;
}
void CBombGameRules::SendMoneyTo(CBasePlayer*recipient)
{
	if(!recipient)return;bool spectator=!Q_stricmp(recipient->TeamID(),SPEC);for(int n=1;n<=gpGlobals->maxClients;n++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(n);if(!p)continue;EnsureMoney(p);bool visible=spectator||p==recipient||(IsValidTeam(p->TeamID())&&!Q_stricmp(p->TeamID(),recipient->TeamID()));MESSAGE_BEGIN(MSG_ONE,gmsgMoney,NULL,recipient->pev);WRITE_BYTE(n);WRITE_LONG(visible?m_money[n]:-1);MESSAGE_END();MESSAGE_BEGIN(MSG_ONE,gmsgHealthInfo,NULL,recipient->pev);WRITE_BYTE(n);WRITE_SHORT(visible?Q_max(0,(int)ceilf(p->pev->health)):-1);MESSAGE_END();}
}
void CBombGameRules::TouchBuyZone(CBasePlayer*p,int team){if(!p)return;int i=p->entindex();m_buyZoneTeam[i]=team;m_buyZoneTouchUntil[i]=gpGlobals->time+0.25f;}
bool CBombGameRules::CanBuy(CBasePlayer*p,bool notify)
{
	if(!p||!p->IsAlive()||m_state!=ACTIVE)return false;int i=p->entindex();int team=!Q_stricmp(p->TeamID(),RED)?1:!Q_stricmp(p->TeamID(),BLUE)?2:0;
	if(gpGlobals->time>m_buyEnd){if(notify)HudNotice(p,"The buy period has ended");return false;}
	bool result=false;CBaseEntity*zone=NULL;while((zone=UTIL_FindEntityByClassname(zone,"func_buyzone"))!=NULL){if((zone->pev->team==0||zone->pev->team==team)&&p->Intersects(zone)){result=true;break;}}
	if(!result&&team)
	{
		const char *spawnClass=team==1?"info_player_red":"info_player_blue";
		CBaseEntity *spawn=NULL;
		bool hasCustomSpawns=false;
		while((spawn=UTIL_FindEntityByClassname(spawn,spawnClass))!=NULL)
		{
			hasCustomSpawns=true;
			if((p->GetAbsOrigin()-spawn->GetAbsOrigin()).Length()<=200.0f){result=true;break;}
		}
		if(!result&&!hasCustomSpawns)
		{
			spawnClass=team==1?"info_player_deathmatch":"info_player_start";
			spawn=NULL;
			while((spawn=UTIL_FindEntityByClassname(spawn,spawnClass))!=NULL)
				if((p->GetAbsOrigin()-spawn->GetAbsOrigin()).Length()<=200.0f){result=true;break;}
		}
	}
	if(!result&&notify)HudNotice(p,"You must be in your team's buy zone");return result;
}
void CBombGameRules::CloseBuyMenu(CBasePlayer*p){if(!p)return;m_buyMenuActive[p->entindex()]=false;MESSAGE_BEGIN(MSG_ONE,gmsgShowMenu,NULL,p->pev);WRITE_SHORT(0);WRITE_CHAR(0);WRITE_BYTE(FALSE);WRITE_STRING("");MESSAGE_END();}
void CBombGameRules::ShowBuyMenu(CBasePlayer*p,int page)
{
	if(!CanBuy(p))return;EnsureMoney(p);int i=p->entindex();m_buyMenuActive[i]=true;m_buyMenuPage[i]=page;char menu[768];int keys=(1<<9);
	if(page==0){keys|=0xff;Q_snprintf(menu,sizeof(menu),"Buy Menu  $%d\n\n1. Pistols\n2. Shotguns\n3. Submachineguns\n4. Assault Rifles\n5. Sniper Rifles\n6. Buy Primary ammo\n7. Buy Secondary ammo\n8. Equipment\n\n0. Exit",m_money[i]);}
	else if(page==1){const bool red=!Q_stricmp(p->TeamID(),RED),blue=!Q_stricmp(p->TeamID(),BLUE);keys|=0x3f;Q_snprintf(menu,sizeof(menu),"Pistols  $%d\n\n1. Glock 18 - $300\n2. Beretta - $400\n%s%s5. Desert Eagle .50 AE - $800\n6. Raging Bull - $900\n\n0. Exit",m_money[i],red?"3. Colt 1911 - $400\n":(blue?"3. USP .45 - $500\n":""),red?"4. P229 - $650\n":(blue?"4. Five-SeveN - $700\n":""));}
	else if(page==4){const bool red=!Q_stricmp(p->TeamID(),RED);keys|=red?((1<<0)|(1<<1)|(1<<3)):((1<<0)|(1<<2)|(1<<3));Q_snprintf(menu,sizeof(menu),"Assault Rifles  $%d\n\n%s\n\n0. Exit",m_money[i],red?"1. IMI Galil - $1800\n2. AK-47 - $2700\n4. SG 552 - $3000":"1. FA MAS - $1950\n3. M4 - $3100\n4. AUG - $3300");}
	else if(page==3){const bool red=!Q_stricmp(p->TeamID(),RED);keys|=0xf;Q_snprintf(menu,sizeof(menu),"Submachineguns  $%d\n\n%s3. UMP .45 - $1550\n%s\n0. Exit",m_money[i],red?"1. Mac-10 - $1150\n2. MP-5 - $1200\n":"1. TMP - $1250\n2. MP-5 SD - $1550\n",red?"4. PP-19 Bizon - $2000\n":"4. P-90 - $2350\n");}
	else if(page>=2&&page<=5){keys|=page==5?0x7:1;static const char*names[]={"","","Shotgun - $1200","","","M24 - $1800"};if(page==5)Q_snprintf(menu,sizeof(menu),"Sniper Rifles  $%d\n\n1. %s\n2. M72 LAW - $900\n3. M60 - $4250\n\n0. Exit",m_money[i],names[page]);else Q_snprintf(menu,sizeof(menu),"%s  $%d\n\n1. %s\n\n0. Exit",page==2?"Shotguns":"Submachineguns",m_money[i],names[page]);}
	else if(!Q_stricmp(p->TeamID(),RED)){keys|=0x5f;Q_snprintf(menu,sizeof(menu),"Equipment  $%d\n\n1. Armor - $650\n2. Helmet / Armor + Helmet - $350 / $1000\n3. Flashbang - $200\n4. HE Grenade - $300\n5. Gas Grenade - $300\n\n7. Night Vision Goggles - $300\n\n0. Exit",m_money[i]);}
	else{keys|=0x7f;Q_snprintf(menu,sizeof(menu),"Equipment  $%d\n\n1. Armor - $650\n2. Helmet / Armor + Helmet - $350 / $1000\n3. Flashbang - $200\n4. HE Grenade - $300\n5. Gas Grenade - $300\n6. Defuse Kit - $400\n7. Night Vision Goggles - $300\n\n0. Exit",m_money[i]);}
	MESSAGE_BEGIN(MSG_ONE,gmsgShowMenu,NULL,p->pev);WRITE_SHORT(keys);WRITE_CHAR(-1);WRITE_BYTE(FALSE);WRITE_STRING(menu);MESSAGE_END();
}
bool CBombGameRules::BuyWeapon(CBasePlayer*p,const char*classname,int weaponId,int basePrice)
{
	if(!CanBuy(p))return false;EnsureMoney(p);int i=p->entindex();int price=basePrice;
	if(m_money[i]<price){HudNotice(p,"Not enough money");return false;}
	if(HasWeaponId(p,weaponId)){HudNotice(p,"You already own this weapon");return false;}
	AddMoney(p,-price);p->GiveNamedItem(classname);if(!HasWeaponId(p,weaponId)){AddMoney(p,price);HudNotice(p,"Weapon purchase failed");return false;}m_weaponPurchased[i][weaponId]=true;m_weaponFired[i][weaponId]=false;return true;
}
bool CBombGameRules::BuyAmmo(CBasePlayer*p,bool primary,bool buyAll)
{
	if(!CanBuy(p))return false;EnsureMoney(p);int index=p->entindex(),spent=0,eligible=0;
	for(int slot=0;slot<MAX_ITEM_TYPES;slot++)for(CBasePlayerItem*it=p->m_rgpPlayerItems[slot];it;it=it->m_pNext)
	{
		CBasePlayerWeapon*w=dynamic_cast<CBasePlayerWeapon*>(it);if(!w||!w->pszAmmo1())continue;int id=w->iWeaponID(),cost=0;
		if(id==WEAPON_GLOCK18||id==WEAPON_BERETTA||id==WEAPON_P229||id==WEAPON_FIVESEVEN||id==WEAPON_USP||id==WEAPON_RAGINGBULL||id==WEAPON_COLT1911||id==WEAPON_DEAGLE){if(primary)continue;cost=id==WEAPON_GLOCK18?30:id==WEAPON_BERETTA?20:id==WEAPON_P229?30:id==WEAPON_FIVESEVEN?40:id==WEAPON_USP?25:id==WEAPON_COLT1911?20:55;}else{if(!primary)continue;switch(id){case WEAPON_BIZON:cost=70;break;case WEAPON_P90:cost=60;break;case WEAPON_UMP:cost=35;break;case WEAPON_MP5A3:case WEAPON_MP5SD:case WEAPON_MAC10:case WEAPON_TMP:cost=30;break;case WEAPON_M3:cost=5;break;case WEAPON_FAMAS:cost=55;break;case WEAPON_M4:case WEAPON_GALIL:case WEAPON_SG552:case WEAPON_AUG:cost=60;break;case WEAPON_M24:cost=125;break;case WEAPON_AK47:cost=50;break;case WEAPON_M60:cost=200;break;default:continue;}}
		eligible++;int ammoType=p->GetAmmoIndex(w->pszAmmo1());while(true)
		{
			int before=ammoType>=0?p->m_rgAmmo[ammoType]:0;for(int magazine=0;magazine<MAX_WEAPONS*6;magazine++)if(p->m_rgMagazineAmmoTypes[magazine]==ammoType)before+=p->m_rgMagazineRounds[magazine];
			const int purchaseRounds=id==WEAPON_M3?7:Q_max(1,w->iMaxClip());
			const int roundsToBuy=id==WEAPON_M3?Q_min(purchaseRounds,Q_max(0,w->iMaxAmmo1()-(ammoType>=0?p->m_rgAmmo[ammoType]:0))):purchaseRounds;
			const int purchaseCost=id==WEAPON_M3?roundsToBuy*cost:cost;
			if(roundsToBuy<=0||m_money[index]-spent<purchaseCost)break;
			p->GiveAmmo(purchaseRounds,(char*)w->pszAmmo1(),w->iMaxAmmo1());
			int after=ammoType>=0?p->m_rgAmmo[ammoType]:0;for(int magazine=0;magazine<MAX_WEAPONS*6;magazine++)if(p->m_rgMagazineAmmoTypes[magazine]==ammoType)after+=p->m_rgMagazineRounds[magazine];
			if(after<=before)break;spent+=id==WEAPON_M3?(after-before)*cost:cost;
			if(!buyAll){AddMoney(p,-spent);EMIT_SOUND(p->edict(),CHAN_ITEM,"items/9mmclip1.wav",1.0f,ATTN_NORM);return true;}
		}
	}
	if(spent>0){AddMoney(p,-spent);EMIT_SOUND(p->edict(),CHAN_ITEM,"items/9mmclip1.wav",1.0f,ATTN_NORM);return true;}HudNotice(p,eligible?(m_money[index]>0?"Ammo is full":"Not enough money"):"No suitable weapon");return false;
}
bool CBombGameRules::BuyEquipment(CBasePlayer*p,int slot)
{
	if(!CanBuy(p))return false;int i=p->entindex(),price=0;const char*weapon=NULL;
	switch(slot){case 1:price=650;break;case 2:if(p->m_bHasHelmet){HudNotice(p,"You already have a helmet");return false;}price=p->pev->armorvalue>0?350:1000;break;case 3:price=200;weapon="weapon_flashbang";break;case 4:price=300;weapon="weapon_handgrenade";break;case 5:price=300;weapon="weapon_gasgrenade";break;case 6:price=400;if(Q_stricmp(p->TeamID(),BLUE)){HudNotice(p,"Only Blue can buy a defuse kit");return false;}if(m_hasDefuseKit[i]){HudNotice(p,"You already have a defuse kit");return false;}break;case 7:price=300;if(m_hasNightVision[i]){HudNotice(p,"You already have night vision");return false;}break;default:return false;}
	EnsureMoney(p);if(m_money[i]<price){HudNotice(p,"Not enough money");return false;}
	if(slot==1){if(p->pev->armorvalue>=100){HudNotice(p,"Armor is full");return false;}p->pev->armorvalue=100;}
	else if(slot==2){if(p->pev->armorvalue<=0)p->pev->armorvalue=100;p->m_bHasHelmet=TRUE;}
	else if(weapon){const char*ammoName=slot==3?"Flashbang":slot==4?"Hand Grenade":"GasGrenade";int limit=slot==3?FLASHBANG_MAX_CARRY:1;int ammoType=p->GetAmmoIndex(ammoName),before=ammoType>=0?p->m_rgAmmo[ammoType]:0;if(before>=limit){HudNotice(p,"Grenade limit reached");return false;}p->GiveNamedItem(weapon);int after=ammoType>=0?p->m_rgAmmo[ammoType]:0;if(after<=before){HudNotice(p,"Grenade purchase failed");return false;}}else if(slot==6)m_hasDefuseKit[i]=true;else m_hasNightVision[i]=true;AddMoney(p,-price);return true;
}
void CBombGameRules::SelectBuyMenu(CBasePlayer*p,int slot)
{
	int i=p->entindex(),page=m_buyMenuPage[i];if(slot==0||slot==10){CloseBuyMenu(p);return;}if(!CanBuy(p)){CloseBuyMenu(p);return;}
	if(page==0){if(slot>=1&&slot<=5){ShowBuyMenu(p,slot);return;}if(slot==6){if(BuyAmmo(p,true)){CloseBuyMenu(p);return;}}else if(slot==7){if(BuyAmmo(p,false)){CloseBuyMenu(p);return;}}else if(slot==8){ShowBuyMenu(p,8);return;}}
	else if(page==1&&(slot>=1&&slot<=6)){bool bought=false;if(slot==1)bought=BuyWeapon(p,"weapon_glock18",WEAPON_GLOCK18,300);else if(slot==2)bought=BuyWeapon(p,"weapon_beretta",WEAPON_BERETTA,400);else if(slot==3){if(!Q_stricmp(p->TeamID(),RED))bought=BuyWeapon(p,"weapon_1911",WEAPON_COLT1911,400);else if(!Q_stricmp(p->TeamID(),BLUE))bought=BuyWeapon(p,"weapon_usp",WEAPON_USP,500);}else if(slot==4){if(!Q_stricmp(p->TeamID(),RED))bought=BuyWeapon(p,"weapon_p229",WEAPON_P229,650);else if(!Q_stricmp(p->TeamID(),BLUE))bought=BuyWeapon(p,"weapon_fiveseven",WEAPON_FIVESEVEN,700);}else if(slot==5)bought=BuyWeapon(p,"weapon_deagle",WEAPON_DEAGLE,800);else bought=BuyWeapon(p,"weapon_ragingbull",WEAPON_RAGINGBULL,900);if(bought){CloseBuyMenu(p);return;}}
	else if(page==4&&slot==1&&!Q_stricmp(p->TeamID(),RED)){if(BuyWeapon(p,"weapon_galil",WEAPON_GALIL,1800)){CloseBuyMenu(p);return;}}
	else if(page==4&&slot==1&&!Q_stricmp(p->TeamID(),BLUE)){if(BuyWeapon(p,"weapon_famas",WEAPON_FAMAS,1950)){CloseBuyMenu(p);return;}}
	else if(page==4&&slot==4&&!Q_stricmp(p->TeamID(),RED)){if(BuyWeapon(p,"weapon_sg552",WEAPON_SG552,3000)){CloseBuyMenu(p);return;}}
	else if(page==4&&slot==4&&!Q_stricmp(p->TeamID(),BLUE)){if(BuyWeapon(p,"weapon_aug",WEAPON_AUG,3300)){CloseBuyMenu(p);return;}}
	else if(page==4&&((slot==2&&!Q_stricmp(p->TeamID(),RED))||(slot==3&&!Q_stricmp(p->TeamID(),BLUE)))){bool bought=slot==2?BuyWeapon(p,"weapon_ak47",WEAPON_AK47,2700):BuyWeapon(p,"weapon_m4",WEAPON_M4,3100);if(bought){CloseBuyMenu(p);return;}}
	else if(page==5&&slot==3){if(BuyWeapon(p,"weapon_m60",WEAPON_M60,4250)){CloseBuyMenu(p);return;}}
	else if(page==5&&slot==2){if(BuyWeapon(p,"weapon_m72",WEAPON_M72,900)){CloseBuyMenu(p);return;}}
	else if(page==3&&slot==1&&!Q_stricmp(p->TeamID(),RED)){if(BuyWeapon(p,"weapon_mac10",WEAPON_MAC10,1150)){CloseBuyMenu(p);return;}}
	else if(page==3&&slot==2&&!Q_stricmp(p->TeamID(),RED)){if(BuyWeapon(p,"weapon_mp5a3",WEAPON_MP5A3,1200)){CloseBuyMenu(p);return;}}
	else if(page==3&&slot==2&&!Q_stricmp(p->TeamID(),BLUE)){if(BuyWeapon(p,"weapon_mp5sd",WEAPON_MP5SD,1550)){CloseBuyMenu(p);return;}}
	else if(page==3&&slot==1&&!Q_stricmp(p->TeamID(),BLUE)){if(BuyWeapon(p,"weapon_tmp",WEAPON_TMP,1250)){CloseBuyMenu(p);return;}}
	else if(page==3&&slot==3){if(BuyWeapon(p,"weapon_ump",WEAPON_UMP,1550)){CloseBuyMenu(p);return;}}
	else if(page==3&&slot==4&&!Q_stricmp(p->TeamID(),BLUE)){if(BuyWeapon(p,"weapon_p90",WEAPON_P90,2350)){CloseBuyMenu(p);return;}}
	else if(page==3&&slot==4&&!Q_stricmp(p->TeamID(),RED)){if(BuyWeapon(p,"weapon_bizon",WEAPON_BIZON,2000)){CloseBuyMenu(p);return;}}
	else if((page==2||page==5)&&slot==1){bool bought=page==2?BuyWeapon(p,"weapon_m3",WEAPON_M3,1200):BuyWeapon(p,"weapon_m24",WEAPON_M24,1800);if(bought){CloseBuyMenu(p);return;}}else if(page==8&&BuyEquipment(p,slot)){CloseBuyMenu(p);return;}ShowBuyMenu(p,page);
}
bool CBombGameRules::HasDefuseKit(CBasePlayer*p)const{return p&&m_hasDefuseKit[p->entindex()];}
bool CBombGameRules::SellWeapon(CBasePlayer*p)
{
	if(bomb_sellweapon.value<=0){HudNotice(p,"Weapon selling is disabled");return false;}
	if(!CanBuy(p))return false;CBasePlayerWeapon*w=p?dynamic_cast<CBasePlayerWeapon*>(p->m_pActiveItem):NULL;if(!w){HudNotice(p,"Select a weapon to sell");return false;}
	int id=w->iWeaponID(),grenadePrice=id==WEAPON_HANDGRENADE?300:id==WEAPON_FLASHBANG?200:id==WEAPON_GASGRENADE?300:0;
	if(grenadePrice>0){int ammo=w->PrimaryAmmoIndex();if(ammo<0||p->m_rgAmmo[ammo]<=0){HudNotice(p,"No grenade to sell");return false;}--p->m_rgAmmo[ammo];if(p->m_rgAmmo[ammo]==0){p->SelectItem("weapon_crowbar");p->RemoveWeapon(id);w->DestroyItem();}p->SendAmmoUpdate();AddMoney(p,grenadePrice);HudNotice(p,UTIL_VarArgs("Grenade sold for $%d",grenadePrice));return true;}
	int basePrice=0;switch(id){case WEAPON_GLOCK18:basePrice=300;break;case WEAPON_BERETTA:basePrice=400;break;case WEAPON_P229:basePrice=650;break;case WEAPON_FIVESEVEN:basePrice=700;break;case WEAPON_USP:basePrice=500;break;case WEAPON_COLT1911:basePrice=400;break;case WEAPON_RAGINGBULL:basePrice=900;break;case WEAPON_DEAGLE:basePrice=800;break;case WEAPON_M3:basePrice=1200;break;case WEAPON_MP5A3:basePrice=1200;break;case WEAPON_MP5SD:basePrice=1550;break;case WEAPON_MAC10:basePrice=1150;break;case WEAPON_TMP:basePrice=1250;break;case WEAPON_UMP:basePrice=1550;break;case WEAPON_P90:basePrice=2350;break;case WEAPON_BIZON:basePrice=2000;break;case WEAPON_GALIL:basePrice=1800;break;case WEAPON_FAMAS:basePrice=1950;break;case WEAPON_SG552:basePrice=3000;break;case WEAPON_AUG:basePrice=3300;break;case WEAPON_M4:basePrice=3100;break;case WEAPON_M24:basePrice=1800;break;case WEAPON_AK47:basePrice=2700;break;case WEAPON_M60:basePrice=4250;break;default:HudNotice(p,"This weapon cannot be sold");return false;}
	int i=p->entindex();int value=id==WEAPON_GLOCK18?200:id==WEAPON_BERETTA?250:id==WEAPON_COLT1911?((m_weaponPurchased[i][id]&&!m_weaponFired[i][id])?400:250):(id==WEAPON_MAC10||id==WEAPON_TMP||id==WEAPON_UMP||id==WEAPON_P90||id==WEAPON_BIZON||id==WEAPON_GALIL||id==WEAPON_FAMAS||id==WEAPON_SG552||id==WEAPON_AUG)?(!m_weaponFired[i][id]?basePrice:(basePrice*60)/100):((m_weaponPurchased[i][id]&&!m_weaponFired[i][id])?basePrice:(basePrice*60)/100);
	p->SelectItem("weapon_crowbar");
	if(p->m_pActiveItem==w){HudNotice(p,"Cannot switch away from this weapon");return false;}
	p->RemoveWeapon(id);w->DestroyItem();m_weaponPurchased[i][id]=false;m_weaponFired[i][id]=false;AddMoney(p,value);HudNotice(p,UTIL_VarArgs("Weapon sold for $%d",value));return true;
}
bool CBombGameRules::DropMoney(CBasePlayer*p)
{
	if(!p||!p->IsAlive())return false;if(bomb_moneydrop.value<=0){HudNotice(p,"Money dropping is disabled");return false;}EnsureMoney(p);int i=p->entindex(),requested=Q_max(0,(int)bomb_moneydrop_count.value),amount=Q_min(requested,m_money[i]);if(requested<=0){HudNotice(p,"Money drop amount is zero");return false;}if(amount<=0){HudNotice(p,"You have no money to drop");return false;}
	UTIL_MakeVectors(p->pev->v_angle);Vector origin=p->GetAbsOrigin()+p->pev->view_ofs+gpGlobals->v_forward*24;CBaseEntity*money=CBaseEntity::Create("dropped_money",origin,p->pev->angles,p->edict());if(!money){HudNotice(p,"Could not drop money");return false;}money->pev->iuser1=amount;money->pev->velocity=gpGlobals->v_forward*200+gpGlobals->v_up*100;money->pev->avelocity=Vector(120,180,90);money->pev->fuser1=gpGlobals->time+0.5f;AddMoney(p,-amount);return true;
}
void CBombGameRules::ObserveWeaponFire(CBasePlayer*p)
{
	if(!p)return;int i=p->entindex();if(gpGlobals->time>=m_nextNVGSync[i]){m_nextNVGSync[i]=gpGlobals->time+0.5f;MESSAGE_BEGIN(MSG_ONE,gmsgNVGOwned,NULL,p->pev);WRITE_BYTE(m_hasNightVision[i]?1:0);MESSAGE_END();}if(gpGlobals->time>=m_nextMoneySync[i]){m_nextMoneySync[i]=gpGlobals->time+1.0f;SendMoneyTo(p);}CBasePlayerWeapon*w=dynamic_cast<CBasePlayerWeapon*>(p->m_pActiveItem);int id=w?w->iWeaponID():0,clip=w?w->m_pWeaponContext->m_iClip:-1;
	if(id>0&&id<MAX_WEAPONS&&m_lastObservedWeapon[i]==id&&clip>=0&&clip<m_lastObservedClip[i])m_weaponFired[i][id]=true;m_lastObservedWeapon[i]=id;m_lastObservedClip[i]=clip;
}
void CBombGameRules::AwardRoundMoney(bool red,int winnerReward)
{
	if(m_roundMoneyAwarded)return;m_roundMoneyAwarded=true;
	if(red){m_redLossStreak=0;m_blueLossStreak++;}else{m_blueLossStreak=0;m_redLossStreak++;}int loserBonus=Q_min(3000,1500+500*(Q_max(m_redLossStreak,m_blueLossStreak)-1));
	for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p||!IsValidTeam(p->TeamID()))continue;bool winner=red?!Q_stricmp(p->TeamID(),RED):!Q_stricmp(p->TeamID(),BLUE);AddMoney(p,winner?winnerReward:loserBonus);}
}
void CBombGameRules::CaptureEquipment(CBasePlayer*p,EquipmentSnapshot&out){out=EquipmentSnapshot();if(!p)return;out.valid=true;out.armor=p->pev->armorvalue;out.helmet=p->m_bHasHelmet!=FALSE;memcpy(out.ammo,p->m_rgAmmo,sizeof(out.ammo));memcpy(out.magazineRounds,p->m_rgMagazineRounds,sizeof(out.magazineRounds));memcpy(out.magazineCapacities,p->m_rgMagazineCapacities,sizeof(out.magazineCapacities));memcpy(out.magazineAmmoTypes,p->m_rgMagazineAmmoTypes,sizeof(out.magazineAmmoTypes));for(int slot=0;slot<MAX_ITEM_TYPES;slot++)for(CBasePlayerItem*item=p->m_rgpPlayerItems[slot];item;item=item->m_pNext){int id=item->iWeaponID();if(id<=0||id>=MAX_WEAPONS||FClassnameIs(item->pev,"weapon_bomb"))continue;Q_strncpy(out.weaponClass[id],STRING(item->pev->classname),sizeof(out.weaponClass[id]));CBasePlayerWeapon*w=dynamic_cast<CBasePlayerWeapon*>(item);out.clips[id]=w?w->m_pWeaponContext->m_iClip:WEAPON_NOCLIP;if(id==WEAPON_USP&&w)out.uspSilenced=static_cast<CUSPWeaponContext*>(w->m_pWeaponContext.get())->IsSilenced();if(id==WEAPON_M4&&w)out.m4Silenced=static_cast<CM4WeaponContext*>(w->m_pWeaponContext.get())->IsSilenced();if(id==WEAPON_FAMAS&&w)out.famasBurst=static_cast<CFamasWeaponContext*>(w->m_pWeaponContext.get())->IsBurstMode();if(item==p->m_pActiveItem)out.activeWeapon=id;if(item==p->m_pLastItem)out.lastWeapon=id;}}
void CBombGameRules::RestoreEquipment(CBasePlayer*p,const EquipmentSnapshot&in){if(!p||!in.valid)return;p->RemoveAllItems(TRUE);p->AddWeapon(WEAPON_SUIT);for(int id=1;id<MAX_WEAPONS;id++)if(in.weaponClass[id][0])p->GiveNamedItem(in.weaponClass[id]);memcpy(p->m_rgAmmo,in.ammo,sizeof(in.ammo));memcpy(p->m_rgMagazineRounds,in.magazineRounds,sizeof(in.magazineRounds));memcpy(p->m_rgMagazineCapacities,in.magazineCapacities,sizeof(in.magazineCapacities));memcpy(p->m_rgMagazineAmmoTypes,in.magazineAmmoTypes,sizeof(in.magazineAmmoTypes));p->pev->armorvalue=in.armor;p->m_bHasHelmet=in.helmet?TRUE:FALSE;for(int slot=0;slot<MAX_ITEM_TYPES;slot++)for(CBasePlayerItem*item=p->m_rgpPlayerItems[slot];item;item=item->m_pNext){int id=item->iWeaponID();CBasePlayerWeapon*w=dynamic_cast<CBasePlayerWeapon*>(item);if(w&&id>0&&id<MAX_WEAPONS)w->m_pWeaponContext->m_iClip=in.clips[id];if(id==WEAPON_USP&&w)static_cast<CUSPWeaponContext*>(w->m_pWeaponContext.get())->SetSilenced(in.uspSilenced);if(id==WEAPON_M4&&w)static_cast<CM4WeaponContext*>(w->m_pWeaponContext.get())->SetSilenced(in.m4Silenced);if(id==WEAPON_FAMAS&&w)static_cast<CFamasWeaponContext*>(w->m_pWeaponContext.get())->SetBurstMode(in.famasBurst);if(id==in.lastWeapon)p->m_pLastItem=item;}if(in.activeWeapon>0&&in.activeWeapon<MAX_WEAPONS&&in.weaponClass[in.activeWeapon][0])p->SelectItem(in.weaponClass[in.activeWeapon]);p->SendAmmoUpdate();p->SendMagazineUpdate();}
void CBombGameRules::CaptureGroundWeapons(GroundWeaponSnapshot*out,int&count)
{
	count=0;
	CBaseEntity*entity=NULL;
	while(count<64&&(entity=UTIL_FindEntityByClassname(entity,"weaponbox"))!=NULL)
	{
		CWeaponBox*box=static_cast<CWeaponBox*>(entity);
		GroundWeaponSnapshot&s=out[count];s=GroundWeaponSnapshot();s.valid=true;
		s.origin=box->GetAbsOrigin();s.angles=box->GetAbsAngles();s.velocity=box->GetAbsVelocity();
		Q_strncpy(s.model,STRING(box->pev->model),sizeof(s.model));
		for(int slot=0;slot<MAX_ITEM_TYPES&&s.weaponCount<MAX_WEAPONS;slot++)
			for(CBasePlayerItem*item=box->m_rgpPlayerItems[slot];item&&s.weaponCount<MAX_WEAPONS;item=item->m_pNext)
			{
				if(FClassnameIs(item->pev,"weapon_bomb"))continue;
				int n=s.weaponCount++;
				Q_strncpy(s.weaponClass[n],STRING(item->pev->classname),sizeof(s.weaponClass[n]));
				CBasePlayerWeapon*w=dynamic_cast<CBasePlayerWeapon*>(item);
				s.clips[n]=w?w->m_pWeaponContext->m_iClip:WEAPON_NOCLIP;
				if(w&&w->iWeaponID()==WEAPON_M4)
					s.m4Silenced[n]=static_cast<CM4WeaponContext*>(w->m_pWeaponContext.get())->IsSilenced();
			}
		for(int a=0;a<MAX_AMMO_SLOTS&&s.ammoCount<MAX_AMMO_SLOTS;a++)if(!FStringNull(box->m_rgiszAmmo[a])&&box->m_rgAmmo[a]>0)
		{
			int n=s.ammoCount++;
			Q_strncpy(s.ammoName[n],STRING(box->m_rgiszAmmo[a]),sizeof(s.ammoName[n]));
			s.ammoAmount[n]=box->m_rgAmmo[a];
		}
		count++;
	}
}
void CBombGameRules::RestoreGroundWeapons(const GroundWeaponSnapshot*in,int count)
{
	for(int i=0;i<count;i++)
	{
		const GroundWeaponSnapshot&s=in[i];if(!s.valid)continue;
		CWeaponBox*box=static_cast<CWeaponBox*>(CBaseEntity::Create("weaponbox",s.origin,s.angles,NULL));if(!box)continue;
		if(s.model[0])SET_MODEL(box->edict(),s.model);
		for(int w=0;w<s.weaponCount;w++)
		{
			CBasePlayerItem*item=dynamic_cast<CBasePlayerItem*>(CBaseEntity::Create((char*)s.weaponClass[w],s.origin,s.angles,NULL));
			if(!item)continue;
			CBasePlayerWeapon*weapon=dynamic_cast<CBasePlayerWeapon*>(item);
			if(weapon)weapon->m_pWeaponContext->m_iClip=s.clips[w];
			if(weapon&&weapon->iWeaponID()==WEAPON_M4)
				static_cast<CM4WeaponContext*>(weapon->m_pWeaponContext.get())->SetSilenced(s.m4Silenced[w]);
			if(!box->PackWeapon(item))UTIL_Remove(item);
		}
		for(int a=0;a<s.ammoCount;a++)box->PackAmmo(ALLOC_STRING(s.ammoName[a]),s.ammoAmount[a]);
		box->SetAbsVelocity(s.velocity);
		if(box->IsEmpty())box->Kill();
	}
}
void CBombGameRules::ExecuteForcedRestart(){m_redWins=m_roundStartRedWins;m_blueWins=m_roundStartBlueWins;m_completedRounds=m_roundStartCompletedRounds;m_executingForcedRestart=true;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p||!m_roundStartTeam[i])continue;ApplyTeamChoice(p,m_roundStartTeam[i],false);}m_forcedRestartAt=0;m_lastForcedRestartSecond=-1;StartRound();m_executingForcedRestart=false;}
void CBombGameRules::ResetMatchForPopulationStart()
{
	m_redWins=0;m_blueWins=0;m_completedRounds=0;m_roundStartRedWins=0;m_roundStartBlueWins=0;m_roundStartCompletedRounds=0;
	m_redLossStreak=0;m_blueLossStreak=0;m_roundMoneyAwarded=false;m_roundStartGroundWeaponCount=0;m_transitionGroundWeaponCount=0;
	const int startMoney=bound(0,(int)CVAR_GET_FLOAT("mp_startmoney"),MoneyLimit());
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;
		p->RemoveAllItems(TRUE);
		memset(p->m_rgAmmo,0,sizeof(p->m_rgAmmo));
		memset(p->m_rgMagazineRounds,0,sizeof(p->m_rgMagazineRounds));
		memset(p->m_rgMagazineCapacities,0,sizeof(p->m_rgMagazineCapacities));
		memset(p->m_rgMagazineAmmoTypes,0,sizeof(p->m_rgMagazineAmmoTypes));
		p->pev->armorvalue=0;p->m_bHasHelmet=FALSE;
		m_money[i]=startMoney;m_moneyInitialized[i]=true;
		m_hasDefuseKit[i]=false;m_hasNightVision[i]=false;m_diedThisRound[i]=false;
		memset(m_weaponFired[i],0,sizeof(m_weaponFired[i]));memset(m_weaponPurchased[i],0,sizeof(m_weaponPurchased[i]));
		m_roundStartEquipment[i]=EquipmentSnapshot();m_transitionEquipment[i]=EquipmentSnapshot();
	}
	for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(p)SendMoneyTo(p);}
}
void CBombGameRules::EnsureWinTargets(){const char*names[]={RED,BLUE};for(int i=0;i<2;i++){bool found=false;CBaseEntity*e=NULL;while((e=UTIL_FindEntityByClassname(e,"bomb_win_target"))!=NULL)if(!Q_stricmp(STRING(e->pev->targetname),names[i])){found=true;break;}if(found)continue;e=CBaseEntity::Create("bomb_win_target",g_vecZero,g_vecZero,NULL);if(e)e->pev->targetname=ALLOC_STRING(names[i]);}}
void CBombGameRules::TeamNotice(const char*team,const char*text){hudtextparms_t h={};h.x=-1.0f;h.y=0.60f;h.effect=0;h.r1=255;h.g1=190;h.b1=40;h.a1=255;h.r2=h.r1;h.g2=h.g1;h.b2=h.b1;h.a2=255;h.fadeinTime=0.1f;h.fadeoutTime=0.3f;h.holdTime=2.5f;h.channel=2;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(p&&!Q_stricmp(p->TeamID(),team))UTIL_HudMessage(p,h,text);}}
void CBombGameRules::CheckElimination(){if(m_state!=ACTIVE)return;int ra=0,ba=0,rt=0,bt=0;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;if(!Q_stricmp(p->TeamID(),RED)){rt++;if(p->IsAlive())ra++;}if(!Q_stricmp(p->TeamID(),BLUE)){bt++;if(p->IsAlive())ba++;}}if(rt&&ra==0&&!m_bomb){AwardRoundMoney(false);EndRound(false,"Blue wins");}else if(bt&&ba==0){AwardRoundMoney(true);EndRound(true,"Red wins");}}
void CBombGameRules::EndRound(bool red,const char*){if(m_state!=ACTIVE)return;m_state=FINISHED;m_nextRound=gpGlobals->time+7;m_completedRounds++;if(red)m_redWins++;else m_blueWins++;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(p)CLIENT_COMMAND(p->edict(),red?"spk radio/terwin.wav\n":"spk radio/ctwin.wav\n");}hudtextparms_t h={};h.x=-1.0f;h.y=0.27f;h.effect=0;h.r1=red?255:70;h.g1=red?60:130;h.b1=red?60:255;h.a1=255;h.r2=h.r1;h.g2=h.g1;h.b2=h.b1;h.a2=255;h.fadeinTime=0.15f;h.fadeoutTime=0.5f;h.holdTime=4.0f;h.channel=3;UTIL_HudMessageAll(h,UTIL_VarArgs("%s wins!",red?m_team1Name:m_team2Name));SendHud();int roundLimit=(int)CVAR_GET_FLOAT("mp_roundlimit"),winLimit=(int)CVAR_GET_FLOAT("mp_winlimit");bool limitReached=(roundLimit>0&&m_completedRounds>=roundLimit)||(winLimit>0&&(m_redWins>=winLimit||m_blueWins>=winLimit));ALERT(at_console,"Bomb mode limits: round %d/%d, score %d:%d, win limit %d%s\n",m_completedRounds,roundLimit,m_redWins,m_blueWins,winLimit,limitReached?" -- map change":"");if(limitReached&&m_forcedRestartAt<=0){hudtextparms_t match={};match.x=-1.0f;match.y=0.38f;match.effect=0;match.r1=255;match.g1=255;match.b1=255;match.a1=255;match.r2=255;match.g2=255;match.b2=255;match.a2=255;match.fadeinTime=0.2f;match.fadeoutTime=0.5f;match.holdTime=14.0f;match.channel=4;const char *result=m_redWins==m_blueWins?"The match is a draw!":UTIL_VarArgs("%s won the match!",m_redWins>m_blueWins?m_team1Name:m_team2Name);UTIL_HudMessageAll(match,result);GoToIntermission();}}
void CBombGameRules::SetKnifeAsLastItem(CBasePlayer*p){if(!p)return;for(int slot=0;slot<MAX_ITEM_TYPES;slot++)for(CBasePlayerItem*item=p->m_rgpPlayerItems[slot];item;item=item->m_pNext)if(FClassnameIs(item->pev,"weapon_crowbar")){p->m_pLastItem=item;return;}}
void CBombGameRules::GiveCarrier(){if(m_bomb)return;CBaseEntity*existing=NULL;while((existing=UTIL_FindEntityByClassname(existing,"weapon_bomb"))!=NULL)if(!FBitSet(existing->pev->flags,FL_KILLME))return;CBasePlayer*list[64];int n=0;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(p&&p->IsAlive()&&!Q_stricmp(p->TeamID(),RED))list[n++]=p;}if(n){CBasePlayer*carrier=list[RANDOM_LONG(0,n-1)];m_givingCarrier=true;carrier->GiveNamedItem("weapon_bomb");m_givingCarrier=false;SetKnifeAsLastItem(carrier);}}
void CBombGameRules::StartRound()
{
	for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(m_executingForcedRestart)m_transitionEquipment[i]=m_roundStartEquipment[i];else{if(p&&p->IsAlive()&&!m_diedThisRound[i])p->PrepareWeaponsForNextRound();CaptureEquipment(p,m_transitionEquipment[i]);if(m_diedThisRound[i]){m_transitionEquipment[i].armor=0;m_transitionEquipment[i].helmet=false;}}}
	if(m_executingForcedRestart)
	{
		m_transitionGroundWeaponCount=m_roundStartGroundWeaponCount;
		memcpy(m_transitionGroundWeapons.get(),m_roundStartGroundWeapons.get(),sizeof(GroundWeaponSnapshot)*64);
	}
	else m_transitionGroundWeaponCount=0;
	CBaseEntity *box=NULL;
	while((box=UTIL_FindEntityByClassname(box,"weaponbox"))!=NULL)
		static_cast<CWeaponBox*>(box)->Kill();
	// Standalone dropped/map MP-5 SD entities are not necessarily packed in a
	// weaponbox. Remove only ground copies; held weapons are restored for
	// surviving players through the normal equipment snapshot.
	CBaseEntity *groundMP5SD=NULL;
	while((groundMP5SD=UTIL_FindEntityByClassname(groundMP5SD,"weapon_mp5sd"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundMP5SD);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundTMP=NULL;
	while((groundTMP=UTIL_FindEntityByClassname(groundTMP,"weapon_tmp"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundTMP);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundUMP=NULL;
	while((groundUMP=UTIL_FindEntityByClassname(groundUMP,"weapon_ump"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundUMP);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundP90=NULL;
	while((groundP90=UTIL_FindEntityByClassname(groundP90,"weapon_p90"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundP90);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundBizon=NULL;
	while((groundBizon=UTIL_FindEntityByClassname(groundBizon,"weapon_bizon"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundBizon);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundGalil=NULL;
	while((groundGalil=UTIL_FindEntityByClassname(groundGalil,"weapon_galil"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundGalil);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundFamas=NULL;
	while((groundFamas=UTIL_FindEntityByClassname(groundFamas,"weapon_famas"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundFamas);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundSG552=NULL;
	while((groundSG552=UTIL_FindEntityByClassname(groundSG552,"weapon_sg552"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundSG552);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	CBaseEntity *groundAUG=NULL;
	while((groundAUG=UTIL_FindEntityByClassname(groundAUG,"weapon_aug"))!=NULL)
	{
		CBasePlayerWeapon *weapon=dynamic_cast<CBasePlayerWeapon*>(groundAUG);
		if(weapon&&!weapon->m_pPlayer)UTIL_Remove(weapon);
	}
	const char *cleanup[]={"planted_bomb","weapon_bomb","weapon_m72","spent_m72","dropped_money","item_dropped_magazine",
		"grenade","flashbang_grenade","gas_grenade","rpg_rocket","m72_rocket","hvr_rocket","crossbow_bolt",
		"hornet","monster_satchel","timed_satchel_bomb","timed_satchel_preview","monster_tripmine",
		"monster_snark","spark_shower","gib"};
	for(int c=0;c<(int)(sizeof(cleanup)/sizeof(cleanup[0]));c++)
	{
		CBaseEntity *e=NULL;
		while((e=UTIL_FindEntityByClassname(e,cleanup[c]))!=NULL) UTIL_Remove(e);
	}
	// Packed grenade weapons are hidden child entities of a weaponbox. Remove
	// them explicitly as well: CWeaponBox::Kill normally defers their deletion.
	const char *droppedGrenadeClasses[]={"weapon_handgrenade","weapon_flashbang","weapon_gasgrenade"};
	for(int c=0;c<3;c++)
	{
		CBaseEntity *grenadeWeapon=NULL;
		while((grenadeWeapon=UTIL_FindEntityByClassname(grenadeWeapon,droppedGrenadeClasses[c]))!=NULL)
			if(FBitSet(grenadeWeapon->pev->spawnflags,SF_NORESPAWN))UTIL_Remove(grenadeWeapon);
	}
	m_bomb=NULL;m_state=ACTIVE;m_waitingForPlayers=false;m_roundMoneyAwarded=false;m_forcedRestartAt=0;m_lastForcedRestartSecond=-1;
	m_freezeEnd=gpGlobals->time+Q_max(0.0f,bomb_freezetime.value);m_buyEnd=gpGlobals->time+Q_max(0.0f,bomb_buytime.value);m_roundEnd=m_freezeEnd+BombRoundSeconds();m_c4Timer=Q_max(1.0f,bomb_c4timer.value);m_lastSecond=-1;
	MESSAGE_BEGIN(MSG_ALL,gmsgKillDecals);WRITE_ENTITY(0);MESSAGE_END();
	UTIL_FireTargets("round_reset",g_pWorld,g_pWorld,USE_TOGGLE,0);
	ResetLightsForBombRound();
	const char *reGameResetClasses[]={"cycler_sprite","multi_manager","env_render","env_spark","trigger_push","trigger_once","func_wall_toggle","func_healthcharger","func_recharge","trigger_hurt","multisource","env_beam","env_laser","trigger_auto","trigger_multiple","func_tracktrain","func_vehicle","func_train","armoury_entity","ambient_generic","env_sprite"};
	for(int c=0;c<(int)(sizeof(reGameResetClasses)/sizeof(reGameResetClasses[0]));c++)
	{
		CBaseEntity *mapperEntity=NULL;
		while((mapperEntity=UTIL_FindEntityByClassname(mapperEntity,reGameResetClasses[c]))!=NULL)
		{
			if(FClassnameIs(mapperEntity->pev,"multi_manager")&&FBitSet(mapperEntity->pev->spawnflags,0x80000000))continue; // threaded clone
			ClearBits(mapperEntity->pev->flags,FL_KILLME);
			mapperEntity->SetThink(NULL);mapperEntity->SetTouch(NULL);mapperEntity->DontThink();
			DispatchSpawn(mapperEntity->edict());
		}
	}
	CBaseEntity *breakable=NULL;
	while((breakable=UTIL_FindEntityByClassname(breakable,"func_breakable"))!=NULL)
		static_cast<CBreakable*>(breakable)->ResetForBombRound();
	breakable=NULL;
	while((breakable=UTIL_FindEntityByClassname(breakable,"func_pushable"))!=NULL)
		static_cast<CBreakable*>(breakable)->ResetForBombRound();
	const char *doorClasses[]={"func_door","func_door_rotating","func_water"};
	for(int c=0;c<3;c++)
	{
		CBaseEntity *door=NULL;
		while((door=UTIL_FindEntityByClassname(door,doorClasses[c]))!=NULL)
			static_cast<CBaseDoor*>(door)->ResetForBombRound();
	}
	const char *buttonClasses[]={"func_button","func_rot_button"};
	for(int c=0;c<2;c++)
	{
		CBaseEntity *button=NULL;
		while((button=UTIL_FindEntityByClassname(button,buttonClasses[c]))!=NULL)
			static_cast<CBaseButton*>(button)->ResetForBombRound();
	}
	const char *platformClasses[]={"func_plat","func_platform"};
	for(int c=0;c<2;c++)
	{
		CBaseEntity *platform=NULL;
		while((platform=UTIL_FindEntityByClassname(platform,platformClasses[c]))!=NULL)
			static_cast<CFuncPlat*>(platform)->ResetForBombRound();
	}
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CBasePlayer *p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;
		if(m_pendingTeam[i]){int s=m_pendingTeam[i];m_pendingTeam[i]=0;ApplyTeamChoice(p,s,false);}
		if(IsValidTeam(p->TeamID())){p->RemoveAllItems(TRUE);respawn(p,FALSE);p->SetAbsVelocity(g_vecZero);p->pev->basevelocity=g_vecZero;p->pev->avelocity=g_vecZero;p->m_flFallVelocity=0;ClearBits(p->pev->flags,FL_BASEVELOCITY|FL_FROZEN);UTIL_DropToFloor(p);p->ResetBombRoundEffects();RestoreEquipment(p,m_transitionEquipment[i]);if(!p->HasNamedPlayerItem("weapon_crowbar"))p->GiveNamedItem("weapon_crowbar");if(p->m_rgpPlayerItems[2]==NULL){const bool blue=!Q_stricmp(p->TeamID(),BLUE);p->GiveNamedItem(blue?"weapon_beretta":"weapon_glock18");m_weaponPurchased[i][blue?WEAPON_BERETTA:WEAPON_GLOCK18]=false;}p->m_bBombFreezeTime=m_freezeEnd>gpGlobals->time;g_engfuncs.pfnSetPhysicsKeyValue(p->edict(),"bombfreeze",p->m_bBombFreezeTime?"1":"0");}
		m_diedThisRound[i]=false;
	}
	for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;MESSAGE_BEGIN(MSG_ALL,gmsgTeamInfo);WRITE_BYTE(i);WRITE_STRING(p->TeamID());MESSAGE_END();SendScoreStatus(p,p->IsAlive()?1:0);}
	RestoreGroundWeapons(m_transitionGroundWeapons.get(),m_transitionGroundWeaponCount);
	GiveCarrier();m_roundStartRedWins=m_redWins;m_roundStartBlueWins=m_blueWins;m_roundStartCompletedRounds=m_completedRounds;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);m_roundStartTeam[i]=!p?0:!Q_stricmp(p->TeamID(),RED)?1:!Q_stricmp(p->TeamID(),BLUE)?2:!Q_stricmp(p->TeamID(),SPEC)?6:0;CaptureEquipment(p,m_roundStartEquipment[i]);}CaptureGroundWeapons(m_roundStartGroundWeapons.get(),m_roundStartGroundWeaponCount);if(m_freezeEnd<=gpGlobals->time){for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(p)CLIENT_COMMAND(p->edict(),"spk radio/go.wav\n");}m_freezeEnd=0;}SendHud();
}
void CBombGameRules::SendHud(){int roundSecs=m_freezeEnd>gpGlobals->time?(int)ceilf(m_freezeEnd-gpGlobals->time):(int)ceilf(m_roundEnd-gpGlobals->time);int secs=m_waitingForPlayers?-1:(m_state==ACTIVE?(m_bomb?static_cast<CObjectiveBomb*>((CBaseEntity*)m_bomb)->SecondsRemaining():roundSecs):(int)ceilf(m_nextRound-gpGlobals->time));int restartSecs=m_forcedRestartAt>0?Q_max(0,(int)ceilf(m_forcedRestartAt-gpGlobals->time)):-1;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;CBasePlayer*watched=m_spectatorTarget[i]?(CBasePlayer*)UTIL_PlayerByIndex(m_spectatorTarget[i]):NULL;bool seesRed=!Q_stricmp(p->TeamID(),RED)||(!Q_stricmp(p->TeamID(),SPEC)&&watched&&!Q_stricmp(watched->TeamID(),RED));int shown=(m_waitingForPlayers||(m_bomb&&!seesRed))?-1:Q_max(0,secs);MESSAGE_BEGIN(MSG_ONE,gmsgBombHud,NULL,p->pev);WRITE_SHORT(shown);WRITE_SHORT(m_redWins);WRITE_SHORT(m_blueWins);WRITE_BYTE(m_state);WRITE_STRING(m_team1Name);WRITE_STRING(m_team2Name);WRITE_SHORT(restartSecs);MESSAGE_END();bool plantHint=m_state==ACTIVE&&!m_bomb&&p->IsAlive()&&!Q_stricmp(p->TeamID(),RED)&&p->HasNamedPlayerItem("weapon_bomb")&&CanPlantBomb(p);if(plantHint||m_plantHintShown[i]){MESSAGE_BEGIN(MSG_ONE,gmsgPickupHint,NULL,p->pev);WRITE_STRING(plantHint?"Bomb Plant":"");MESSAGE_END();m_plantHintShown[i]=plantHint;}}}
void CBombGameRules::StartTeamMenuCamera(CBasePlayer*p)
{
	if(!p)return;int idx=p->entindex();m_teamMenuCameraActive[idx]=true;m_teamMenuCameraIndex[idx]=-1;m_nextTeamMenuCamera[idx]=0;
	p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);p->pev->iuser1=p->pev->iuser2=0;SelectNextTeamMenuCamera(p);
}
void CBombGameRules::StopTeamMenuCamera(CBasePlayer*p)
{
	if(!p)return;int idx=p->entindex();m_teamMenuCameraActive[idx]=false;m_teamMenuCameraIndex[idx]=0;m_nextTeamMenuCamera[idx]=0;SET_VIEW(p->edict(),p->edict());
}
void CBombGameRules::SelectNextTeamMenuCamera(CBasePlayer*p)
{
	if(!p)return;int idx=p->entindex(),count=0;CBaseEntity*camera=NULL;
	while((camera=UTIL_FindEntityByClassname(camera,"trigger_camera"))!=NULL)count++;
	if(count<=0){m_teamMenuCameraActive[idx]=false;SET_VIEW(p->edict(),p->edict());return;}
	m_teamMenuCameraIndex[idx]=(m_teamMenuCameraIndex[idx]+1)%count;camera=NULL;
	for(int n=0;n<=m_teamMenuCameraIndex[idx];n++)camera=UTIL_FindEntityByClassname(camera,"trigger_camera");
	if(!camera){m_teamMenuCameraActive[idx]=false;SET_VIEW(p->edict(),p->edict());return;}
	CBaseEntity*target=camera->GetNextTarget();if(target)camera->SetAbsAngles(UTIL_VecToAngles(target->EyePosition()-camera->GetAbsOrigin()));
	// Xash only sends a non-player view entity to the client when it has a
	// model index. trigger_camera::Use does the same thing; the camera remains
	// invisible because trigger_camera is spawned with renderamt 0.
	if(camera->pev->modelindex==0&&p->GetModel()[0])SET_MODEL(camera->edict(),p->GetModel());
	p->pev->movetype=MOVETYPE_NONE;p->pev->iuser1=p->pev->iuser2=0;SET_VIEW(p->edict(),camera->edict());m_nextTeamMenuCamera[idx]=gpGlobals->time+4.0f;
}
void CBombGameRules::UpdateTeamMenuCameras()
{
	for(int i=1;i<=gpGlobals->maxClients;i++){if(!m_teamMenuCameraActive[i])continue;CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p||m_hasTeamChoice[i]){if(p)StopTeamMenuCamera(p);else m_teamMenuCameraActive[i]=false;continue;}if(gpGlobals->time>=m_nextTeamMenuCamera[i])SelectNextTeamMenuCamera(p);}
}
void CBombGameRules::SendPingInfo()
{
	if(gpGlobals->time<m_nextPingUpdate)return;
	m_nextPingUpdate=gpGlobals->time+1.0f;
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;
		int ping=0,loss=0;PLAYER_CNX_STATS(p->edict(),&ping,&loss);
		MESSAGE_BEGIN(MSG_ALL,gmsgPingInfo);WRITE_BYTE(i);WRITE_SHORT(Q_max(0,ping));MESSAGE_END();
	}
}
void CBombGameRules::SelectSpectatorTarget(CBasePlayer*spectator,int direction)
{
	if(!spectator)return;int idx=spectator->entindex(),start=m_spectatorTarget[idx];
	for(int step=1;step<=gpGlobals->maxClients;step++)
	{
		int n=start+direction*step;while(n<1)n+=gpGlobals->maxClients;while(n>gpGlobals->maxClients)n-=gpGlobals->maxClients;
		CBasePlayer*t=(CBasePlayer*)UTIL_PlayerByIndex(n);
		if(t&&t!=spectator&&t->IsAlive()&&IsValidTeam(t->TeamID())){m_spectatorTarget[idx]=n;return;}
	}
	m_spectatorTarget[idx]=0;
}
void CBombGameRules::SendSpectatorHud(CBasePlayer*s,CBasePlayer*t)
{
	if(!s||!t)return;
	MESSAGE_BEGIN(MSG_ONE,gmsgSpecTarget,NULL,s->pev);WRITE_BYTE(1);WRITE_BYTE(t->entindex());MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE,gmsgHealth,NULL,s->pev);WRITE_BYTE(Q_max(0,Q_min(255,(int)t->pev->health)));MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE,gmsgBattery,NULL,s->pev);WRITE_SHORT((int)t->pev->armorvalue);MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE,gmsgHelmet,NULL,s->pev);WRITE_BYTE(t->m_bHasHelmet!=FALSE);MESSAGE_END();
	byte weapons[MAX_WEAPON_BYTES]={};
	for(int slot=0;slot<MAX_ITEM_TYPES;slot++)for(CBasePlayerItem*item=t->m_rgpPlayerItems[slot];item;item=item->m_pNext){int id=item->iWeaponID();if(id>0&&id<MAX_WEAPONS)weapons[id>>3]|=(1<<(id&7));}
	MESSAGE_BEGIN(MSG_ONE,gmsgWeapons,NULL,s->pev);for(int i=0;i<MAX_WEAPON_BYTES;i++)WRITE_BYTE(weapons[i]);MESSAGE_END();
	for(int i=0;i<MAX_AMMO_SLOTS;i++){MESSAGE_BEGIN(MSG_ONE,gmsgAmmoX,NULL,s->pev);WRITE_BYTE(i);WRITE_BYTE(Q_max(0,Q_min(254,t->m_rgAmmo[i])));MESSAGE_END();}
	CBasePlayerWeapon*w=t->m_pActiveItem?dynamic_cast<CBasePlayerWeapon*>(t->m_pActiveItem):NULL;int weaponId=w?w->iWeaponID():0,clip=w?w->m_pWeaponContext->m_iClip:0;
	MESSAGE_BEGIN(MSG_ONE,gmsgCurWeapon,NULL,s->pev);WRITE_BYTE(1);WRITE_BYTE(weaponId);WRITE_BYTE(clip);MESSAGE_END();
	int magazineType=w&&w->m_pWeaponContext->UsesMagazineInventory()?weaponId:0;
	MESSAGE_BEGIN(MSG_ONE,gmsgMagazines,NULL,s->pev);WRITE_BYTE(magazineType);for(int n=0;n<6;n++){int pos=magazineType*6+n;WRITE_BYTE(magazineType?t->m_rgMagazineRounds[pos]:0);WRITE_BYTE(magazineType?t->m_rgMagazineCapacities[pos]:0);}WRITE_BYTE(0);MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE,gmsgMoney,NULL,s->pev);WRITE_BYTE(s->entindex());WRITE_LONG(m_money[t->entindex()]);MESSAGE_END();
	MESSAGE_BEGIN(MSG_ONE,gmsgNVGOwned,NULL,s->pev);WRITE_BYTE(m_hasNightVision[t->entindex()]?1:0);MESSAGE_END();
	s->pev->viewmodel=t->pev->viewmodel;s->pev->weaponanim=t->pev->weaponanim;s->pev->fov=t->pev->fov;
}
void CBombGameRules::UpdateSpectators()
{
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CBasePlayer*s=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!s||Q_stricmp(s->TeamID(),SPEC))continue;
		CBasePlayer*t=m_spectatorTarget[i]?(CBasePlayer*)UTIL_PlayerByIndex(m_spectatorTarget[i]):NULL;
		if((s->m_afButtonPressed&IN_ATTACK)||!t||!t->IsAlive()||!IsValidTeam(t->TeamID())){SelectSpectatorTarget(s,1);t=m_spectatorTarget[i]?(CBasePlayer*)UTIL_PlayerByIndex(m_spectatorTarget[i]):NULL;}
		else if(s->m_afButtonPressed&IN_ATTACK2){SelectSpectatorTarget(s,-1);t=m_spectatorTarget[i]?(CBasePlayer*)UTIL_PlayerByIndex(m_spectatorTarget[i]):NULL;}
		if(!t){SET_VIEW(s->edict(),s->edict());s->pev->iuser1=s->pev->iuser2=0;s->pev->movetype=MOVETYPE_NOCLIP;if(gpGlobals->time>=m_nextSpectatorHud[i]){m_nextSpectatorHud[i]=gpGlobals->time+0.25f;MESSAGE_BEGIN(MSG_ONE,gmsgSpecTarget,NULL,s->pev);WRITE_BYTE(0);WRITE_BYTE(0);MESSAGE_END();}continue;}
		s->pev->movetype=MOVETYPE_NONE;s->pev->iuser1=OBS_IN_EYE;s->pev->iuser2=t->entindex();SET_VIEW(s->edict(),t->edict());
		if(gpGlobals->time>=m_nextSpectatorHud[i]){m_nextSpectatorHud[i]=gpGlobals->time+0.1f;SendSpectatorHud(s,t);}
	}
}
void CBombGameRules::Think()
{
	extern DLL_GLOBAL BOOL g_fGameOver;
	if(g_fGameOver)
	{
		CHalfLifeMultiplay::Think();
		return;
	}
	EnsureWinTargets();
	SendPingInfo();
	UpdateTeamMenuCameras();
	UpdateSpectators();
	if(Q_stricmp(bomb_team1name.string,m_team1Name))Q_strncpy(g_szNextTeam1Name,bomb_team1name.string,sizeof(g_szNextTeam1Name));
	if(Q_stricmp(bomb_team2name.string,m_team2Name))Q_strncpy(g_szNextTeam2Name,bomb_team2name.string,sizeof(g_szNextTeam2Name));
	int red=0,blue=0,total=0;
	const bool freezeActive=m_state==ACTIVE&&m_freezeEnd>gpGlobals->time;
	for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;if(IsValidTeam(p->TeamID())&&p->IsAlive()){ClearBits(p->pev->flags,FL_FROZEN);p->m_bBombFreezeTime=freezeActive;g_engfuncs.pfnSetPhysicsKeyValue(p->edict(),"bombfreeze",freezeActive?"1":"0");}else{p->m_bBombFreezeTime=FALSE;g_engfuncs.pfnSetPhysicsKeyValue(p->edict(),"bombfreeze","0");}ObserveWeaponFire(p);if(m_buyMenuActive[i]&&!CanBuy(p,false))CloseBuyMenu(p);if(IsValidTeam(p->TeamID()))total++;if(!Q_stricmp(p->TeamID(),RED))red++;if(!Q_stricmp(p->TeamID(),BLUE))blue++;}
	if(m_freezeEnd>0&&!freezeActive){m_freezeEnd=0;for(int i=1;i<=gpGlobals->maxClients;i++){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(p)CLIENT_COMMAND(p->edict(),"spk radio/go.wav\n");}SendHud();}
	if(total==0){if(!m_waitingForPlayers){m_waitingForPlayers=true;m_state=FINISHED;m_nextRound=gpGlobals->time+999999.0f;m_forcedRestartAt=0;SendHud();}return;}
	if(m_waitingForPlayers){StartRound();return;}
	if(m_forcedRestartAt>0)
	{
		int remaining=(int)ceilf(m_forcedRestartAt-gpGlobals->time);
		if(remaining<=0){ExecuteForcedRestart();return;}
		if(remaining!=m_lastForcedRestartSecond){m_lastForcedRestartSecond=remaining;SendHud();}
		if(m_state==FINISHED){SendHud();return;}
	}
	if(total<=1)m_lowPopulation=true;
	if(m_lowPopulation&&red>0&&blue>0&&m_populationRestartAt<=0){m_populationRestartAt=gpGlobals->time+5;HudNoticeAll("Game starting!");}
	if(m_populationRestartAt>0&&gpGlobals->time>=m_populationRestartAt){m_populationRestartAt=0;m_lowPopulation=false;ResetMatchForPopulationStart();StartRound();return;}
	if(m_state==FINISHED){if(gpGlobals->time>=m_nextRound)StartRound();else SendHud();return;}
	if(!m_bomb&&gpGlobals->time>=m_roundEnd){AwardRoundMoney(false);EndRound(false,"Blue wins");return;}
	int sec=(int)gpGlobals->time;if(sec!=m_lastSecond){m_lastSecond=sec;SendHud();CheckElimination();}
}
