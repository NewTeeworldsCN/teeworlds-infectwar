/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/vmath.h>

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/
class IGameController
{
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];

	class CGameContext *m_pGameServer;
	class IServer *m_pServer;

protected:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }

	struct CSpawnEval
	{
		CSpawnEval()
		{
			m_Got = false;
			m_FriendlyTeam = -1;
			m_Pos = vec2(100,100);
		}

		vec2 m_Pos;
		bool m_Got;
		int m_FriendlyTeam;
		float m_Score;
	};

	float EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos);
	void EvaluateSpawnType(CSpawnEval *pEval, int Type);
	bool EvaluateSpawn(class CPlayer *pP, vec2 *pPos);

	void CycleMap();
	void ResetGame();

	char m_aMapWish[128];


	int m_RoundStartTick;
	int m_GameOverTick;
	int m_SuddenDeath;

	int m_aTeamscore[2];

	int m_Warmup;
	int m_UnpauseTimer;
	int m_RoundCount;

	int m_GameFlags;
	int m_UnbalancedTick;
	bool m_ForceBalanced;

public:
	const char *m_pGameType;

	bool IsTeamplay() const;
	bool IsGameOver() const { return m_GameOverTick != -1; }

	IGameController(class CGameContext *pGameServer);
	virtual ~IGameController();

	virtual void DoWincheck();

	void DoWarmup(int Seconds);
	void TogglePause();

	virtual void StartRound();
	void EndRound();
	void ChangeMap(const char *pToMap);

	virtual bool IsFriendlyFire(int ClientID1, int ClientID2);

	bool IsForceBalanced();

	/*

	*/
	virtual bool CanBeMovedOnBalance(int ClientID);

	virtual void Tick();

	virtual void Snap(int SnappingClient);

	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.

		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.

		Returns:
			bool?
	*/
	virtual bool OnEntity(int Index, vec2 Pos);

	/*
		Function: on_CCharacter_spawn
			Called when a CCharacter spawns into the game world.

		Arguments:
			chr - The CCharacter that was spawned.
	*/
	virtual void OnCharacterSpawn(class CCharacter *pChr);

	/*
		Function: on_CCharacter_death
			Called when a CCharacter in the world dies.

		Arguments:
			victim - The CCharacter that died.
			killer - The player that killed it.
			weapon - What weapon that killed it. Can be -1 for undefined
				weapon when switching team or player suicides.
	*/
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

	virtual void OnCharacterWeaponFired(class CCharacter *pChr, int Weapon, vec2 FirePos, vec2 Direction);


	virtual void OnPlayerInfoChange(class CPlayer *pP);

	virtual void OnPlayerSendEmoticon(class CPlayer *pPlayer, int Emoticon) {};

	//
	virtual bool CanSpawn(int Team, vec2 *pPos);

	virtual bool PlayerCanPickup(class CPlayer *pPlayer) { return true; };
	/*

	*/
	virtual const char *GetTeamName(int Team);
	virtual int GetAutoTeam(int NotThisID);
	virtual bool CanJoinTeam(int Team, int NotThisID);
	bool CheckTeamBalance();
	bool CanChangeTeam(CPlayer *pPplayer, int JoinTeam);
	int ClampTeam(int Team);

	virtual void PostReset();
};

#endif
