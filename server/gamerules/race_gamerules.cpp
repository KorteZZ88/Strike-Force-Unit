#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "client.h"
#include "game.h"
#include "user_messages.h"
#include "race_gamerules.h"
#include "entities/func_car.h"
#include "entities/func_race.h"

static const int kRacePoints[] = { 10, 8, 6, 5, 4, 3, 2, 1 };
static const char *kRaceTeam = "racer";
static const char *kSpectatorTeam = "spectator";

static CFuncCar *RaceCarByEdict(int index)
{
	if (index <= 0 || index >= gpGlobals->maxEntities) return NULL;
	edict_t *edict = INDEXENT(index);
	if (!edict || edict->free) return NULL;
	CBaseEntity *entity = CBaseEntity::Instance(edict);
	if (!entity || entity->pev->classname == NULL_STRING) return NULL;
	const char *classname = STRING(entity->pev->classname);
	if (Q_stricmp(classname, "func_car") && Q_strnicmp(classname, "car_", 4)) return NULL;
	return static_cast<CFuncCar *>(entity);
}

CRaceGameRules::CRaceGameRules() { PRECACHE_SOUND("plats/train_use1.wav"); ValidateMap(true); }

bool CRaceGameRules::IsRacer(CBasePlayer *p) const { return p && !Q_stricmp(p->TeamID(), kRaceTeam); }
bool CRaceGameRules::IsSpectator(CBasePlayer *p) const { return p && !Q_stricmp(p->TeamID(), kSpectatorTeam); }

const char *CRaceGameRules::SetDefaultPlayerTeam(CBasePlayer *p)
{
	if (IsRacer(p) || IsSpectator(p)) return p->m_szTeamName;
	p->m_szTeamName[0] = 0; return p->m_szTeamName;
}

void CRaceGameRules::ChangePlayerTeam(CBasePlayer *p, const char *team, BOOL, BOOL)
{
	Q_strncpy(p->m_szTeamName, team, TEAM_NAME_LENGTH); const int index=p->entindex();
	g_engfuncs.pfnSetClientKeyValue(index,g_engfuncs.pfnGetInfoKeyBuffer(p->edict()),"team",team);
	MESSAGE_BEGIN(MSG_ALL,gmsgTeamInfo);WRITE_BYTE(index);WRITE_STRING(team);MESSAGE_END();
}

void CRaceGameRules::ShowTeamMenu(CBasePlayer *p)
{
	MESSAGE_BEGIN(MSG_ONE,gmsgShowMenu,NULL,p->pev);WRITE_SHORT((1<<0)|(1<<5));WRITE_CHAR(-1);WRITE_BYTE(FALSE);WRITE_STRING("Choose team\n\n1. Racer\n\n\n\n6. Spectator");MESSAGE_END();
}
void CRaceGameRules::CloseTeamMenu(CBasePlayer *p){MESSAGE_BEGIN(MSG_ONE,gmsgShowMenu,NULL,p->pev);WRITE_SHORT(0);WRITE_CHAR(0);WRITE_BYTE(FALSE);WRITE_STRING("");MESSAGE_END();}

void CRaceGameRules::SelectTeam(CBasePlayer *p,int slot)
{
	if(!p||(slot!=1&&slot!=6))return;int index=p->entindex();CloseTeamMenu(p);m_hasTeamChoice[index]=true;m_spectatorCar[index]=0;m_spectatorMode[index]=0;m_spectatorButtons[index]=(int)p->pev->button;
	if(p->m_pVehicle!=NULL)static_cast<CFuncCar*>(static_cast<CBaseEntity*>(p->m_pVehicle))->ForceRaceExit();SET_VIEW(p->edict(),p->edict());
	if(slot==6){ChangePlayerTeam(p,kSpectatorTeam,FALSE,FALSE);p->RemoveAllItems(TRUE);p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);p->pev->movetype=MOVETYPE_NOCLIP;p->pev->flags|=FL_SPECTATOR;p->pev->iuser3=0;return;}
	p->pev->iuser3=0;
	ChangePlayerTeam(p,kRaceTeam,FALSE,FALSE);ClearBits(p->pev->flags,FL_SPECTATOR);ClearBits(p->m_afPhysicsFlags,PFLAG_OBSERVER);
	if(m_state==WAITING)respawn(p,FALSE);else{p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);p->pev->movetype=MOVETYPE_NOCLIP;}
}

BOOL CRaceGameRules::ClientCommand(CBasePlayer*p,const char*cmd)
{
	if(FStrEq(cmd,"menuselect")){if(CMD_ARGC()>1)SelectTeam(p,atoi(CMD_ARGV(1)));return TRUE;}if(FStrEq(cmd,"chooseteam")||FStrEq(cmd,"changeteam")){ShowTeamMenu(p);return TRUE;}return FALSE;
}

void CRaceGameRules::PlayerSpawn(CBasePlayer*p)
{
	if(!IsRacer(p)){p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);p->pev->movetype=MOVETYPE_NOCLIP;return;}CHalfLifeMultiplay::PlayerSpawn(p);
}

int CRaceGameRules::NowMs() const { return m_state >= RACING ? Q_max(0, (int)((gpGlobals->time - (m_countdownEnd)) * 1000.0f)) : 0; }

void CRaceGameRules::UpdateGameMode(CBasePlayer *p)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, p->edict()); WRITE_BYTE(3); MESSAGE_END();
}

void CRaceGameRules::InitHUD(CBasePlayer *p)
{
	CHalfLifeMultiplay::InitHUD(p);
	ShowTeamMenu(p);
	if(!m_hasTeamChoice[p->entindex()]){p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);p->pev->movetype=MOVETYPE_NOCLIP;}
	SendHud(p);
	SendRaceData();
}

void CRaceGameRules::AutoSeatPlayer(CBasePlayer *player)
{
	if (!player || m_state != WAITING || !player->IsAlive() || player->m_pVehicle != NULL) return;

	CFuncCar *nearest = NULL;
	float nearestDistance = 1e30f;
	for (int i = 1; i < gpGlobals->maxEntities; ++i)
	{
		CFuncCar *car = RaceCarByEdict(i);
		if (!car || car->GetVehicleDriver() != NULL) continue;
		const float distance = (car->GetAbsOrigin() - player->GetAbsOrigin()).Length();
		if (distance < nearestDistance) { nearest = car; nearestDistance = distance; }
	}
	if (nearest) nearest->ForceRaceEnter(player);
}

bool CRaceGameRules::ValidateMap(bool report)
{
	int startFinish = 0; m_maxCheckpoint = 0;
	bool found[256] = {};
	CBaseEntity *e = NULL;
	while ((e = UTIL_FindEntityByClassname(e, "func_race")) != NULL)
	{
		CFuncRace *r = static_cast<CFuncRace *>(e);
		if (r->GetRaceType() == CFuncRace::START_FINISH) ++startFinish;
		if (r->GetRaceType() == CFuncRace::CHECKPOINT && r->GetCheckpointNumber() > 0 && r->GetCheckpointNumber() < 256)
		{ found[r->GetCheckpointNumber()] = true; m_maxCheckpoint = Q_max(m_maxCheckpoint, r->GetCheckpointNumber()); }
	}
	bool ok = startFinish == 1 && m_maxCheckpoint > 0;
	for (int i = 1; i <= m_maxCheckpoint; ++i) if (!found[i]) ok = false;
	if (report && !ok) ALERT(at_console, "RaceLap map error: need exactly one start_finish and checkpoints numbered 1..N without gaps (found start_finish=%d, max checkpoint=%d)\n", startFinish, m_maxCheckpoint);
	return ok;
}

CFuncCar *CRaceGameRules::TouchedCar(CBaseEntity *other) const
{
	if (!other) return NULL;
	CFuncCar *car = dynamic_cast<CFuncCar *>(other);
	if (car) return car;
	CBaseEntity *driver = other->GetVehicleDriver();
	return driver ? dynamic_cast<CFuncCar *>(other) : NULL;
}

void CRaceGameRules::Notice(CBasePlayer *p, const char *text)
{
	if (!p) return;
	MESSAGE_BEGIN(MSG_ONE, gmsgPickupHint, NULL, p->pev); WRITE_STRING(text); MESSAGE_END();
}

int CRaceGameRules::ActiveRacers() const
{
	int n = 0; for (int i = 1; i <= gpGlobals->maxClients; ++i) if (m_racers[i].active && !m_racers[i].dnf && !m_racers[i].finished) ++n; return n;
}

void CRaceGameRules::CountReady(int &ready,int &racers) const
{
	ready=racers=0;for(int i=1;i<=gpGlobals->maxClients;++i){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!IsRacer(p)||!p->IsAlive())continue;++racers;if(p->m_pVehicle!=NULL)++ready;}
}

void CRaceGameRules::StartRace()
{
	if (m_state != WAITING) { ALERT(at_console, "race_start: a heat is already active\n"); return; }
	if (!ValidateMap(true)) return;
	int seated=0,racers=0;CountReady(seated,racers);
	if(seated<Q_max(1,(int)race_min_players.value)||seated!=racers){ALERT(at_console,"race_start: need all racers seated and at least %d racers (ready %d/%d)\n",Q_max(1,(int)race_min_players.value),seated,racers);return;}
	BeginCountdown();
}

void CRaceGameRules::BeginCountdown()
{
	m_state = COUNTDOWN; m_countdownEnd = gpGlobals->time + 3.0f; m_finishers = 0;
	for (int i=1;i<=gpGlobals->maxClients;++i)
	{
		CBasePlayer *p=(CBasePlayer*)UTIL_PlayerByIndex(i); Racer &r=m_racers[i];
		r.active=IsRacer(p)&&p->IsAlive()&&p->m_pVehicle!=NULL; r.car=r.active?p->m_pVehicle:NULL; r.armed=r.finished=r.dnf=false; r.nextCheckpoint=1; r.laps=r.place=r.finishMs=r.lastLapMs=r.lapStartMs=0;
		if(r.active) static_cast<CFuncCar *>((CBaseEntity*)r.car)->StartEngineForRace();
		else if(IsRacer(p)&&p->IsAlive()) p->StartObserver(p->GetAbsOrigin(),p->pev->v_angle);
	}
	ALERT(at_console,"RaceLap: heat %d countdown started with %d racers\n",m_heat+1,ActiveRacers()); SendHud();
}

void CRaceGameRules::BeginRacing()
{
	m_state=RACING; m_countdownEnd=gpGlobals->time;
	for(int i=1;i<=gpGlobals->maxClients;++i) if(m_racers[i].active&&m_racers[i].car!=NULL) static_cast<CFuncCar *>((CBaseEntity*)m_racers[i].car)->SetRaceLocked(false);
	for (int i=1;i<gpGlobals->maxEntities;++i) { CFuncCar *car=RaceCarByEdict(i); if(!car)continue;if(car->GetVehicleDriver()!=NULL)EMIT_SOUND_DYN(car->edict(),CHAN_ITEM,"plats/train_use1.wav",1.0f,ATTN_NORM,0,PITCH_NORM);else car->RemoveForRace(); }
	SendHud();
}

void CRaceGameRules::RaceTriggerTouched(CBaseEntity *entity, CBaseEntity *other)
{
	if (m_state!=RACING&&m_state!=FINISH_WINDOW) return;
	CFuncRace *trigger=dynamic_cast<CFuncRace *>(entity); CFuncCar *car=TouchedCar(other); if(!trigger||!car) return;
	CBasePlayer *p=dynamic_cast<CBasePlayer *>(car->GetVehicleDriver()); if(!p) return;
	int idx=p->entindex(); Racer &r=m_racers[idx]; if(!r.active||r.finished||r.dnf||r.car!=car) return;
	const int type=trigger->GetRaceType();
	if(type==CFuncRace::START_FINISH)
	{
		if(!r.armed){r.armed=true;r.nextCheckpoint=1;r.lapStartMs=NowMs();Notice(p,"Race started!");SendRaceData();return;}
		if(r.nextCheckpoint<=m_maxCheckpoint)return;
		const int now=NowMs();r.lastLapMs=now-r.lapStartMs;r.lapStartMs=now;++r.laps;r.nextCheckpoint=1;
		if(r.laps>=Q_max(1,(int)race_laps.value))FinishRacer(p);else {if(r.laps==Q_max(1,(int)race_laps.value)-1)Notice(p,"FINAL LAP!");SendRaceData();}
	}
	else if(type==CFuncRace::CHECKPOINT&&r.armed)
	{
		const int n=trigger->GetCheckpointNumber();
		if(n==r.nextCheckpoint){++r.nextCheckpoint;SendRaceData();}
		else if(n>r.nextCheckpoint)Notice(p,UTIL_VarArgs("Checkpoint %d missed!",r.nextCheckpoint));
	}
}

void CRaceGameRules::FinishRacer(CBasePlayer *p)
{
	Racer&r=m_racers[p->entindex()];r.finished=true;r.finishMs=NowMs();r.place=++m_finishers;
	if(r.place<=8)r.points+=kRacePoints[r.place-1];if(r.place==1)++r.wins;if(r.place<=3)++r.podiums;++r.heats;
	if(r.car!=NULL)static_cast<CFuncCar *>((CBaseEntity*)r.car)->SetRaceLocked(true);
	Notice(p,UTIL_VarArgs("FINISHED - place %d",r.place));
	if(m_state==RACING){m_state=FINISH_WINDOW;m_finishDeadline=gpGlobals->time+Q_max(1.0f,race_finish_timeout.value);}
	SendRaceData();
	if(ActiveRacers()==0)EndHeat();
}

void CRaceGameRules::MarkDNF(CBasePlayer *p)
{
	if(!p)return;Racer&r=m_racers[p->entindex()];if(!r.active||r.finished||r.dnf)return;r.dnf=true;++r.dnfs;++r.heats;if(r.car!=NULL)static_cast<CFuncCar *>((CBaseEntity*)r.car)->SetRaceLocked(true);
}

void CRaceGameRules::EndHeat()
{
	if(m_state==RESULTS)return;for(int i=1;i<=gpGlobals->maxClients;++i){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(p)MarkDNF(p);}++m_heat;m_state=RESULTS;m_resultsEnd=gpGlobals->time+8.0f;SendRaceData();SendHud();
}

void CRaceGameRules::RestartRace(){ResetToWaiting();}

void CRaceGameRules::ResetCarsAndPlayers()
{
	for(int i=1;i<gpGlobals->maxEntities;++i){CFuncCar*car=RaceCarByEdict(i);if(car)car->ResetForRace();}
}

void CRaceGameRules::ResetToWaiting()
{
	m_state=WAITING;m_readySince=0;m_finishers=0;
	for(int i=1;i<=gpGlobals->maxClients;++i){Racer&r=m_racers[i];r.active=r.armed=r.finished=r.dnf=false;r.car=NULL;}
	ResetCarsAndPlayers();
	for(int i=1;i<=gpGlobals->maxClients;++i){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(IsRacer(p)&&(FBitSet(p->pev->flags,FL_SPECTATOR)||FBitSet(p->m_afPhysicsFlags,PFLAG_OBSERVER))){ClearBits(p->pev->flags,FL_SPECTATOR);ClearBits(p->m_afPhysicsFlags,PFLAG_OBSERVER);respawn(p,FALSE);AutoSeatPlayer(p);}}
	SendHud();
}

void CRaceGameRules::PlayerKilled(CBasePlayer*p,entvars_t*k,entvars_t*i){MarkDNF(p);CHalfLifeMultiplay::PlayerKilled(p,k,i);}
void CRaceGameRules::ClientDisconnected(edict_t*e){CBasePlayer*p=(CBasePlayer*)CBaseEntity::Instance(e);MarkDNF(p);CHalfLifeMultiplay::ClientDisconnected(e);}
BOOL CRaceGameRules::ClientConnected(edict_t*e,const char*n,const char*a,char reject[128]){int index=ENTINDEX(e);if(index>0&&index<65){m_racers[index]=Racer();m_hasTeamChoice[index]=false;m_spectatorCar[index]=m_spectatorMode[index]=m_spectatorButtons[index]=0;}return CHalfLifeMultiplay::ClientConnected(e,n,a,reject);}

void CRaceGameRules::SendHud(CBasePlayer *only)
{
	int ready=0,racers=0;CountReady(ready,racers);
	for(int i=1;i<=gpGlobals->maxClients;++i){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p||(only&&p!=only))continue;Racer&r=m_racers[i];int countdown=m_state==COUNTDOWN?Q_max(0,(int)ceilf(m_countdownEnd-gpGlobals->time)):0;int remain=m_state==FINISH_WINDOW?Q_max(0,(int)ceilf(m_finishDeadline-gpGlobals->time)):0;
		const int totalLaps=Q_max(1,(int)race_laps.value);const int currentLap=r.armed?Q_min(totalLaps,r.laps+1):0;
		MESSAGE_BEGIN(MSG_ONE,gmsgBombHud,NULL,p->pev);WRITE_SHORT(-32768);WRITE_BYTE(1);WRITE_BYTE(m_state);WRITE_BYTE(Q_max(1,m_heat+1));WRITE_BYTE(Q_max(1,(int)race_heats.value));WRITE_BYTE(currentLap);WRITE_BYTE(totalLaps);WRITE_BYTE(r.armed?Q_max(0,r.nextCheckpoint-1):0);WRITE_BYTE(m_maxCheckpoint);WRITE_BYTE(countdown);WRITE_SHORT(remain);WRITE_LONG(NowMs());WRITE_LONG(r.lastLapMs);WRITE_BYTE(ready);WRITE_BYTE(racers);MESSAGE_END();
		MESSAGE_BEGIN(MSG_ONE,gmsgBombHud,NULL,p->pev);WRITE_SHORT(-32768);WRITE_BYTE(3);WRITE_BYTE(IsSpectator(p)?m_spectatorMode[i]:0);MESSAGE_END();}
}

void CRaceGameRules::SelectSpectatorCar(CBasePlayer*p,int direction)
{
	const int index=p->entindex(),start=m_spectatorCar[index];int found=0;
	for(int step=1;step<gpGlobals->maxEntities;++step){int n=start+direction*step;while(n<=0)n+=gpGlobals->maxEntities-1;while(n>=gpGlobals->maxEntities)n-=gpGlobals->maxEntities-1;CFuncCar*car=RaceCarByEdict(n);if(car&&car->GetVehicleDriver()!=NULL){found=n;break;}}m_spectatorCar[index]=found;
}

void CRaceGameRules::UpdateSpectators()
{
	for(int i=1;i<=gpGlobals->maxClients;++i){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!IsSpectator(p))continue;
		const int buttons=(int)p->pev->button;const int pressed=buttons&~m_spectatorButtons[i];m_spectatorButtons[i]=buttons;
		if(pressed&IN_ATTACK)SelectSpectatorCar(p,1);
		if(pressed&IN_JUMP){m_spectatorMode[i]=(m_spectatorMode[i]+1)%3;if(m_spectatorMode[i]&&!m_spectatorCar[i])SelectSpectatorCar(p,1);}
		CFuncCar*car=RaceCarByEdict(m_spectatorCar[i]);if(!car||car->GetVehicleDriver()==NULL){if(m_spectatorCar[i])SelectSpectatorCar(p,1);car=RaceCarByEdict(m_spectatorCar[i]);}
		if(!car&&m_spectatorMode[i])m_spectatorMode[i]=0;
		p->pev->iuser3=m_spectatorMode[i];
		if(m_spectatorMode[i]==0){SET_VIEW(p->edict(),p->edict());p->pev->movetype=MOVETYPE_NOCLIP;}
		else if(car){SET_VIEW(p->edict(),car->GetVehicleViewEntity()->edict());p->pev->movetype=MOVETYPE_NONE;}
	}
}

void CRaceGameRules::SendRaceData()
{
	for(int i=1;i<=gpGlobals->maxClients;++i){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(!p)continue;Racer&r=m_racers[i];MESSAGE_BEGIN(MSG_ALL,gmsgBombHud);WRITE_SHORT(-32768);WRITE_BYTE(2);WRITE_BYTE(i);WRITE_BYTE(r.place);WRITE_BYTE(r.laps);WRITE_BYTE(r.finished?1:(r.dnf?2:0));WRITE_LONG(r.finishMs);WRITE_LONG(r.laps?r.finishMs/r.laps:0);WRITE_SHORT(r.points);WRITE_BYTE(r.wins);WRITE_BYTE(r.podiums);WRITE_BYTE(r.heats);WRITE_BYTE(r.dnfs);MESSAGE_END();}
}

void CRaceGameRules::Think()
{
	CHalfLifeMultiplay::Think();int teamRacers=0,teamSpectators=0;for(int i=1;i<=gpGlobals->maxClients;++i){CBasePlayer*p=(CBasePlayer*)UTIL_PlayerByIndex(i);if(IsRacer(p))++teamRacers;else if(IsSpectator(p))++teamSpectators;}if(teamRacers>0)m_hadRacers=true;else if(m_hadRacers&&teamSpectators>0){m_hadRacers=false;ALERT(at_console,"RaceLap: last racer left the team; race_restart\n");RestartRace();}UpdateSpectators();if(m_state==WAITING){for(int i=1;i<gpGlobals->maxEntities;++i){CFuncCar*car=RaceCarByEdict(i);if(car)car->PrepareForRace();}int players=0,seated=0;CountReady(seated,players);if(players>=Q_max(1,(int)race_min_players.value)&&players==seated){if(m_readySince<=0)m_readySince=gpGlobals->time;if(gpGlobals->time-m_readySince>=1.0f)StartRace();}else m_readySince=0;}
	else if(m_state==COUNTDOWN&&gpGlobals->time>=m_countdownEnd)BeginRacing();else if(m_state==FINISH_WINDOW&&gpGlobals->time>=m_finishDeadline)EndHeat();else if(m_state==RESULTS&&gpGlobals->time>=m_resultsEnd){if(m_heat>=Q_max(1,(int)race_heats.value))GoToIntermission();else ResetToWaiting();}
	if(gpGlobals->time>=m_nextHud){m_nextHud=gpGlobals->time+0.1f;SendHud();}
}
