#pragma once
#include "gamerules.h"

class CFuncCar;
class CFuncRace;

class CRaceGameRules : public CHalfLifeMultiplay
{
public:
	CRaceGameRules();
	BOOL IsRaceMode() override { return TRUE; }
	BOOL CanPlayerExitVehicle(CBasePlayer *) override { return FALSE; }
	const char *GetGameDescription() override { return "PrimeXT RaceLap"; }
	void Think() override;
	void InitHUD(CBasePlayer *player) override;
	BOOL ClientCommand(CBasePlayer *player, const char *command) override;
	void PlayerSpawn(CBasePlayer *player) override;
	const char *SetDefaultPlayerTeam(CBasePlayer *player) override;
	void ChangePlayerTeam(CBasePlayer *player, const char *team, BOOL kill, BOOL gib) override;
	void UpdateGameMode(CBasePlayer *player) override;
	void ClientDisconnected(edict_t *client) override;
	BOOL ClientConnected(edict_t *entity, const char *name, const char *address, char reject[128]) override;
	void PlayerKilled(CBasePlayer *victim, entvars_t *killer, entvars_t *inflictor) override;
	void RaceTriggerTouched(CBaseEntity *trigger, CBaseEntity *other) override;
	void StartRace() override;
	void RestartRace() override;

private:
	enum State { WAITING, COUNTDOWN, RACING, FINISH_WINDOW, RESULTS };
	struct Racer
	{
		EHANDLE car;
		bool active = false, armed = false, finished = false, dnf = false;
		int nextCheckpoint = 1, laps = 0, place = 0;
		int finishMs = 0, lastLapMs = 0, lapStartMs = 0;
		int points = 0, wins = 0, podiums = 0, heats = 0, dnfs = 0;
	};
	bool ValidateMap(bool report);
	void BeginCountdown();
	void BeginRacing();
	void FinishRacer(CBasePlayer *player);
	void EndHeat();
	void ResetToWaiting();
	void ResetCarsAndPlayers();
	void AutoSeatPlayer(CBasePlayer *player);
	void MarkDNF(CBasePlayer *player);
	void SendHud(CBasePlayer *only = nullptr);
	void SendRaceData();
	void Notice(CBasePlayer *player, const char *text);
	void ShowTeamMenu(CBasePlayer *player);
	void CloseTeamMenu(CBasePlayer *player);
	void SelectTeam(CBasePlayer *player, int slot);
	void UpdateSpectators();
	void SelectSpectatorCar(CBasePlayer *player, int direction);
	bool IsRacer(CBasePlayer *player) const;
	bool IsSpectator(CBasePlayer *player) const;
	void CountReady(int &ready, int &racers) const;
	CFuncCar *TouchedCar(CBaseEntity *other) const;
	int NowMs() const;
	int ActiveRacers() const;

	State m_state = WAITING;
	Racer m_racers[65];
	int m_maxCheckpoint = 0, m_finishers = 0, m_heat = 0;
	float m_countdownEnd = 0, m_finishDeadline = 0, m_resultsEnd = 0;
	float m_readySince = 0, m_nextHud = 0;
	bool m_hasTeamChoice[65] = {};
	int m_spectatorCar[65] = {};
	int m_spectatorMode[65] = {};
	int m_spectatorButtons[65] = {};
	bool m_hadRacers = false;
};
