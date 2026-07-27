#pragma once
#include "teamplay_gamerules.h"
#include <memory>
class CBombGameRules : public CHalfLifeTeamplay
{
public:
	CBombGameRules(); BOOL IsBombMode() override { return TRUE; } const char *GetGameDescription() override { return "Bomb Defusal"; }
	void Think() override; void InitHUD(CBasePlayer *p) override; void UpdateGameMode(CBasePlayer *p) override; BOOL ClientCommand(CBasePlayer *p,const char *cmd) override;
	void ClientUserInfoChanged(CBasePlayer *p, char *infobuffer) override;
	void PlayerSpawn(CBasePlayer *p) override; void PlayerKilled(CBasePlayer *p,entvars_t *k,entvars_t *i) override; void ClientDisconnected(edict_t *p) override;
	BOOL FPlayerCanRespawn(CBasePlayer *) override; BOOL FPlayerCanTakeDamage(CBasePlayer *p,CBaseEntity *a) override;
	BOOL CanHavePlayerItem(CBasePlayer *p,CBasePlayerItem *w) override; void PlayerGotWeapon(CBasePlayer *p,CBasePlayerItem *w) override;
	edict_t *GetPlayerSpawnSpot(CBasePlayer *p) override; int IPointsForKill(CBasePlayer*,CBasePlayer*) override { return 0; }
	int GetTeamIndex(const char *n) override; const char *GetIndexedTeamName(int i) override; BOOL IsValidTeam(const char *n) override;
	const char *SetDefaultPlayerTeam(CBasePlayer *p) override; void ChangePlayerTeam(CBasePlayer *p,const char *n,BOOL kill,BOOL gib) override;
	BOOL CanPlantBomb(CBasePlayer *p) override; void PlantBomb(CBasePlayer *p) override; void BombDefused(CBasePlayer *p) override; void BombExploded(CBaseEntity *b) override;
	void TargetActivated(const char *targetName) override;
	float BombTimerSeconds() const override { return m_c4Timer; }
	void RestartRoundIn(float seconds) override;
	void TouchBuyZone(CBasePlayer *p,int team);
	bool HasDefuseKit(CBasePlayer *p) const;
	bool PickupDroppedMoney(CBasePlayer *p,int amount);
private:
	struct EquipmentSnapshot
	{
		bool valid=false;
		char weaponClass[MAX_WEAPONS][48] = {};
		int clips[MAX_WEAPONS] = {};
		int ammo[MAX_AMMO_SLOTS] = {};
		int magazineRounds[MAX_WEAPONS*6] = {};
		int magazineCapacities[MAX_WEAPONS*6] = {};
		int magazineAmmoTypes[MAX_WEAPONS*6] = {};
		int activeWeapon=0,lastWeapon=0;
		bool uspSilenced=false;
		bool m4Silenced=false;
		float armor=0;
		bool helmet=false;
	};
	struct GroundWeaponSnapshot
	{
		bool valid=false;
		Vector origin,angles,velocity;
		char model[64] = {};
		int weaponCount=0,ammoCount=0;
		char weaponClass[MAX_WEAPONS][48] = {};
		int clips[MAX_WEAPONS] = {};
		bool m4Silenced[MAX_WEAPONS] = {};
		char ammoName[MAX_AMMO_SLOTS][32] = {};
		int ammoAmount[MAX_AMMO_SLOTS] = {};
	};
	enum State { ACTIVE, FINISHED }; State m_state; float m_roundEnd, m_nextRound; int m_redWins,m_blueWins; EHANDLE m_bomb; int m_lastSecond;
	bool m_lowPopulation=true; float m_populationRestartAt=0;
	int m_pendingTeam[65] = {};
	bool m_hasTeamChoice[65] = {};
	bool m_teamMenuCameraActive[65] = {};
	int m_teamMenuCameraIndex[65] = {};
	float m_nextTeamMenuCamera[65] = {};
	bool m_plantHintShown[65] = {};
	bool m_givingCarrier=false;
	bool m_waitingForPlayers=false;
	float m_freezeEnd=0;
	float m_forcedRestartAt=0;
	int m_lastForcedRestartSecond=-1;
	int m_roundStartRedWins=0,m_roundStartBlueWins=0;
	int m_completedRounds=0,m_roundStartCompletedRounds=0;
	int m_roundStartTeam[65] = {};
	std::unique_ptr<EquipmentSnapshot[]> m_roundStartEquipment;
	std::unique_ptr<EquipmentSnapshot[]> m_transitionEquipment;
	std::unique_ptr<GroundWeaponSnapshot[]> m_roundStartGroundWeapons;
	std::unique_ptr<GroundWeaponSnapshot[]> m_transitionGroundWeapons;
	int m_roundStartGroundWeaponCount=0;
	int m_transitionGroundWeaponCount=0;
	bool m_executingForcedRestart=false;
	float m_c4Timer=35.0f;
	char m_team1Name[32] = "Red";
	char m_team2Name[32] = "Blue";
	int m_money[65] = {};
	bool m_moneyInitialized[65] = {};
	int m_moneyTeam[65] = {};
	float m_buyZoneTouchUntil[65] = {};
	int m_buyZoneTeam[65] = {};
	bool m_buyMenuActive[65] = {};
	int m_buyMenuPage[65] = {};
	bool m_hasDefuseKit[65] = {};
	bool m_hasNightVision[65] = {};
	bool m_diedThisRound[65] = {};
	bool m_weaponFired[65][MAX_WEAPONS] = {};
	bool m_weaponPurchased[65][MAX_WEAPONS] = {};
	int m_lastObservedClip[65] = {};
	int m_lastObservedWeapon[65] = {};
	float m_nextNVGSync[65] = {};
	float m_nextMoneySync[65] = {};
	int m_redLossStreak=0,m_blueLossStreak=0;
	bool m_roundMoneyAwarded=false;
	float m_nextPingUpdate=0;
	void SendPingInfo();
	void ResetMatchForPopulationStart();
	void UpdateTeamMenuCameras(); void StartTeamMenuCamera(CBasePlayer *player); void StopTeamMenuCamera(CBasePlayer *player); void SelectNextTeamMenuCamera(CBasePlayer *player);
	void UpdateSpectators(); void SelectSpectatorTarget(CBasePlayer *spectator,int direction); void SendSpectatorHud(CBasePlayer *spectator,CBasePlayer *target);
	void ShowTeamMenu(CBasePlayer *p); void CloseTeamMenu(CBasePlayer *p); void SelectTeam(CBasePlayer *p,int slot); void ApplyTeamChoice(CBasePlayer *p,int slot,bool spawnNow); void EnsureWinTargets(); void CaptureEquipment(CBasePlayer *p,EquipmentSnapshot &out); void RestoreEquipment(CBasePlayer *p,const EquipmentSnapshot &in); void CaptureGroundWeapons(GroundWeaponSnapshot *out,int &count); void RestoreGroundWeapons(const GroundWeaponSnapshot *in,int count); void ExecuteForcedRestart(); void StartRound(); void EndRound(bool red,const char *reason); void SendHud(); void SendScoreStatus(CBasePlayer *p,int status); void GiveCarrier(); void SetKnifeAsLastItem(CBasePlayer *p); void CheckElimination(); void TeamNotice(const char *team,const char *text);
	void EnsureMoney(CBasePlayer *p); void AddMoney(CBasePlayer *p,int amount); void SendMoneyTo(CBasePlayer *recipient); bool CanBuy(CBasePlayer *p,bool notify=true); void ShowBuyMenu(CBasePlayer *p,int page=0); void SelectBuyMenu(CBasePlayer *p,int slot); void CloseBuyMenu(CBasePlayer *p); bool BuyWeapon(CBasePlayer *p,const char *classname,int weaponId,int price); bool BuyAmmo(CBasePlayer *p,bool primary,bool buyAll=true); bool BuyEquipment(CBasePlayer *p,int slot); bool SellWeapon(CBasePlayer *p); bool DropMoney(CBasePlayer *p); void ObserveWeaponFire(CBasePlayer *p); void AwardRoundMoney(bool red,int winnerReward=3000);
};
