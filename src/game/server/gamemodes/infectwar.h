#ifndef GAME_SERVER_GAMEMODES_INFECTWAR_H
#define GAME_SERVER_GAMEMODES_INFECTWAR_H

#include <game/server/gamecontroller.h>

class CGameControllerInfectWar : public IGameController
{
	bool m_aInfects[MAX_CLIENTS];
	int m_aDeathCount[MAX_CLIENTS];

	bool m_aFixLasers[MAX_CLIENTS];
	int m_aFixLaserIDs[MAX_CLIENTS];
	vec2 m_aFixLaserPoints[MAX_CLIENTS];

	int m_InfectionTimer;
	std::vector<vec2> m_vMapTurretPoints;
public:
	CGameControllerInfectWar(class CGameContext *pGameServer);
	~CGameControllerInfectWar();

	void StartInfection();
public:
	/* overrides start */
	bool IsFriendlyFire(int ClientID1, int ClientID2) override;
	bool PlayerCanPickup(class CPlayer *pPlayer) override;
	bool OnEntity(int Index, vec2 Pos) override;

	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;

	void OnCharacterWeaponFired(class CCharacter *pChr, int Weapon, vec2 FirePos, vec2 Direction) override;
	void OnPlayerSendEmoticon(class CPlayer *pPlayer, int Emoticon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Snap(int SnappingClient) override;
	void DoWincheck() override;
	void StartRound() override;
	void Tick() override;
	/* overrides end */
};

#endif // GAME_SERVER_GAMEMODES_INFECTWAR_H
