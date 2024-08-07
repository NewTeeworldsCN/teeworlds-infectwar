#include <engine/shared/config.h>

#include <game/server/gamecontext.h>

#include <game/server/entities/laser.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/projectile.h>
#include <game/server/entities/turret.h>

#include "infectwar.h"

#include <vector>

CGameControllerInfectWar::CGameControllerInfectWar(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "InfectWar fng";

	for(auto& LaserID : m_aFixLaserIDs)
	{
		LaserID = Server()->SnapNewID();
	}

	m_InfectionTimer = g_Config.m_SvInfectionTime * Server()->TickSpeed();

	mem_zero(m_aInfects, sizeof(m_aInfects));
}

CGameControllerInfectWar::~CGameControllerInfectWar()
{
	for(auto& LaserID : m_aFixLaserIDs)
	{
		Server()->SnapFreeID(LaserID);
	}
}

bool CGameControllerInfectWar::IsFriendlyFire(int ClientID1, int ClientID2)
{
	// -2 = only infect take damage
	if(ClientID1 == -2 && !m_aInfects[ClientID2])
		return true;
	if(ClientID2 == -2 && !m_aInfects[ClientID1])
		return true;

	if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
		return false;

	if(m_aInfects[ClientID1] == m_aInfects[ClientID2])
		return true;

	return false;
}

bool CGameControllerInfectWar::PlayerCanPickup(CPlayer *pPlayer)
{
	return !m_aInfects[pPlayer->GetCID()];
}

bool CGameControllerInfectWar::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	int SubType = 0;

	if(Index == ENTITY_SPAWN || Index == ENTITY_SPAWN_RED || Index == ENTITY_SPAWN_BLUE)
		return IGameController::OnEntity(Index, Pos);
	else if(Index == ENTITY_ARMOR_1)
		Type = POWERUP_ARMOR;
	else if(Index == ENTITY_HEALTH_1)
		Type = POWERUP_HEALTH;
	else if(Index == ENTITY_WEAPON_SHOTGUN)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
	}
	else if(Index == ENTITY_WEAPON_GRENADE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
	}
	else if(Index == ENTITY_WEAPON_RIFLE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_RIFLE;
	}
	else if(Index == ENTITY_POWERUP_NINJA && g_Config.m_SvPowerups)
	{
		Type = POWERUP_NINJA;
		SubType = WEAPON_NINJA;
	}
	else if(Index == ENTITY_FLAGSTAND_RED)
	{
		new CTurret(&GameServer()->m_World, Pos, true, random_int(WEAPON_SHOTGUN, WEAPON_GRENADE),
			-2);
		return true;
	}

	if(Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, SubType, true);
		pPickup->m_Pos = Pos;
		return true;
	}

	return false;
}

int CGameControllerInfectWar::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
	bool VictimHadInfect = m_aInfects[pVictim->GetPlayer()->GetCID()];
	if(VictimHadInfect)
	{
		float Spreading[] = {-2.875f, -1.425f, 0.f, 1.425f, 2.875f};
		float Angle = GetAngle(vec2(0.f, -1.f)) + Spreading[random_int(0, 4)];
		CDroppedPickup *pPickup = new CDroppedPickup(&GameServer()->m_World, 
			random_int(0, 1) ? POWERUP_ARMOR : POWERUP_HEALTH,
			0,
			true);
		pPickup->SetVel(GetDir(Angle) * 8.0f);
		pPickup->m_Pos = pVictim->m_Pos;
	}
	else
	{
		// drop weapon
		std::vector<int> DroppedWeapons;
		for(int i = WEAPON_SHOTGUN; i < NUM_WEAPONS; i++)
		{
			if(pVictim->m_aWeapons[i].m_Got)
				DroppedWeapons.push_back(i);
		}

		if(!DroppedWeapons.empty())
		{
			const int Start = -(int) (DroppedWeapons.size() / 2);
			float Spreading[] = {-2.875f, -1.425f, 1.425f, 2.875f};
			for(int i = Start; i < Start + (int) DroppedWeapons.size(); i++)
			{
				float Angle = GetAngle(vec2(0.f, -1.f)) + Spreading[i + 2];
				CDroppedPickup *pPickup = new CDroppedPickup(&GameServer()->m_World, 
					POWERUP_WEAPON,
					DroppedWeapons[i - Start],
					true);
				pPickup->SetVel(GetDir(Angle) * 8.0f);
				pPickup->m_Pos = pVictim->m_Pos;
			}
		}

		{
			float Spreading[] = {-2.875f, -1.425f, 0.f, 1.425f, 2.875f};
			for(int i = 0; i < pVictim->m_Armor; i++)
			{
				float Angle = GetAngle(vec2(0.f, -1.f)) + Spreading[i % 5];
				CDroppedPickup *pPickup = new CDroppedPickup(&GameServer()->m_World, 
					POWERUP_ARMOR,
					0,
					true);
				pPickup->SetVel(GetDir(Angle) * 8.0f);
				pPickup->m_Pos = pVictim->m_Pos;
			}
		}
	}

	if(!m_InfectionTimer)
		m_aInfects[pVictim->GetPlayer()->GetCID()] = true;

	m_aFixLasers[pVictim->GetPlayer()->GetCID()] = false;
	// do scoreing
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;

	if(pKiller == pVictim->GetPlayer())
	{
		pVictim->GetPlayer()->m_Score--; // suicide
	}
	else
	{
		if(m_aInfects[pKiller->GetCID()] && !VictimHadInfect)
			pKiller->m_Score += 3; // infected kill
		else
			pKiller->m_Score++; // human kill
	}
	if(Weapon == WEAPON_SELF)
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;

	return 0;
}

void CGameControllerInfectWar::OnCharacterWeaponFired(CCharacter *pChr, int Weapon, vec2 FirePos, vec2 Direction)
{
	switch(Weapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			pChr->m_NumObjectsHit = 0;
			GameServer()->CreateSound(pChr->m_Pos, SOUND_HAMMER_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(FirePos, pChr->m_ProximityRadius * 0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			int Damage = g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage;
			if(m_aInfects[pChr->GetPlayer()->GetCID()])
				Damage = 12;

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if((pTarget == pChr) || GameServer()->Collision()->IntersectLine(FirePos, pTarget->m_Pos, nullptr, nullptr))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos - FirePos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - FirePos) * pChr->m_ProximityRadius * 0.5f);
				else
					GameServer()->CreateHammerHit(FirePos);

				vec2 Dir;
				if(length(pTarget->m_Pos - pChr->m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - pChr->m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, Damage,
					pChr->GetPlayer()->GetCID(), Weapon);
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				pChr->m_ReloadTimer = Server()->TickSpeed()/3;

		} break;

		case WEAPON_GUN:
		{
		} break;

		case WEAPON_SHOTGUN:
		{
			int ShotSpread = 2;

			for(int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
				float a = GetAngle(Direction) + Spreading[i + 2];

				float v = 1 - (absolute(i) / (float)ShotSpread);
				float Speed = mix((float) GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);

				new CProjectile(&GameServer()->m_World, WEAPON_SHOTGUN,
					pChr->GetPlayer()->GetCID(),
					FirePos,
					vec2(cosf(a), sinf(a)) * Speed,
					(int)(Server()->TickSpeed() * GameServer()->Tuning()->m_ShotgunLifetime),
					1, 0, 0, -1, WEAPON_SHOTGUN);
			}

			GameServer()->CreateSound(pChr->m_Pos, SOUND_SHOTGUN_FIRE);
		} break;

		case WEAPON_GRENADE:
		{
			new CProjectile(&GameServer()->m_World, WEAPON_GRENADE,
				pChr->GetPlayer()->GetCID(),
				FirePos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
				1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

			GameServer()->CreateSound(pChr->m_Pos, SOUND_GRENADE_FIRE);
		} break;

		case WEAPON_RIFLE:
		{
			new CLaser(&GameServer()->m_World, pChr->m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, pChr->GetPlayer()->GetCID());
			GameServer()->CreateSound(pChr->m_Pos, SOUND_RIFLE_FIRE);
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			pChr->m_NumObjectsHit = 0;

			pChr->m_Ninja.m_ActivationDir = Direction;
			pChr->m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			pChr->m_Ninja.m_OldVelAmount = length(pChr->m_Core.m_Vel);

			GameServer()->CreateSound(pChr->m_Pos, SOUND_NINJA_FIRE);
		} break;

	}
}

void CGameControllerInfectWar::OnPlayerSendEmoticon(CPlayer *pPlayer, int Emoticon)
{
	if(!pPlayer)
		return;
	if(!pPlayer->GetCharacter())
		return;
	if(!pPlayer->GetCharacter()->m_Alive)
		return;

	bool Attacker = false;
	if(Emoticon != EMOTICON_GHOST && Emoticon != EMOTICON_QUESTION)
		return;
	if(Emoticon == EMOTICON_GHOST)
		Attacker = true;
	if(!Attacker && (pPlayer->GetCharacter()->m_ActiveWeapon == WEAPON_HAMMER || pPlayer->GetCharacter()->m_ActiveWeapon == WEAPON_GUN))
		return;
	int Cost = (Attacker ? 0 : 1) + pPlayer->GetCharacter()->m_ActiveWeapon;
	if(pPlayer->GetCharacter()->m_Armor < Cost)
		return;

	pPlayer->GetCharacter()->m_Armor -= Cost;
	GameServer()->CreateSound(pPlayer->GetCharacter()->m_Pos, SOUND_CTF_GRAB_PL);

	new CTurret(&GameServer()->m_World, pPlayer->GetCharacter()->m_Pos, Attacker,
		pPlayer->GetCharacter()->m_ActiveWeapon, pPlayer->GetCID());
}

void CGameControllerInfectWar::OnCharacterSpawn(CCharacter *pChr)
{
	// default health
	pChr->IncreaseHealth(10);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, -1);

	if(!m_aInfects[pChr->GetPlayer()->GetCID()])
		pChr->GiveWeapon(WEAPON_GUN, -1);
}

void CGameControllerInfectWar::StartInfection()
{
	std::vector<int> Humans;
	int HumanCount = 0;
	int InfectedCount = 0;
	for(auto& pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		if(m_aInfects[pPlayer->GetCID()])
			InfectedCount++;
		else
		{
			HumanCount++;
			Humans.push_back(pPlayer->GetCID());
		}
	}

	if(Humans.size() < 2)
		return;

	int PlayersCount = HumanCount + InfectedCount;
	int NeedInfect = -InfectedCount;
	if(PlayersCount < 4)
		NeedInfect += 1;
	else if(PlayersCount < 8)
		NeedInfect += 2;
	else
		NeedInfect += 3;

	for(int i = 0; i < NeedInfect; i++)
	{
		int InfectID = Humans[random_int(0, Humans.size() - 1)];
		GameServer()->m_apPlayers[InfectID]->KillCharacter();
	}
}

void CGameControllerInfectWar::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = (CNetObj_GameInfo *)Server()->SnapNewItem(NETOBJTYPE_GAMEINFO, 0, sizeof(CNetObj_GameInfo));
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if(GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;

	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;
	pGameInfoObj->m_WarmupTimer = m_InfectionTimer;

	pGameInfoObj->m_ScoreLimit = 0;
	pGameInfoObj->m_TimeLimit = g_Config.m_SvTimelimit;

	pGameInfoObj->m_RoundNum = (str_length(g_Config.m_SvMaprotation) && g_Config.m_SvRoundsPerMap) ? g_Config.m_SvRoundsPerMap : 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount+1;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_aFixLasers[i] && GameServer()->GetPlayerChar(i))
		{
			CCharacter *pChr = GameServer()->GetPlayerChar(i);

			CNetObj_Laser *pLaser = (CNetObj_Laser *)Server()->SnapNewItem(NETOBJTYPE_LASER, m_aFixLaserIDs[i], sizeof(CNetObj_Laser));
			if(!pLaser)
				return;

			pLaser->m_X = (int)	m_aFixLaserPoints[i].x;
			pLaser->m_Y = (int) m_aFixLaserPoints[i].y;
			pLaser->m_FromX = (int) pChr->m_Pos.x;
			pLaser->m_FromY = (int) pChr->m_Pos.y;
			pLaser->m_StartTick = Server()->Tick();
		}
	}
}

void CGameControllerInfectWar::DoWincheck()
{
	if(m_GameOverTick != -1 || m_Warmup || GameServer()->m_World.m_ResetRequested)
	{
		return;
	}

	int HumanCount = 0;
	int InfectedCount = 0;
	for(auto& pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;
		if(m_aInfects[pPlayer->GetCID()])
			InfectedCount++;
		else
			HumanCount++;
	}

	if(InfectedCount + HumanCount < 2)
	{
		m_InfectionTimer++;
		m_RoundStartTick++;
		return;
	}

	if(!HumanCount && InfectedCount)
	{
		GameServer()->SendChatTarget(-1, "!| Anytime, it's zombie time...");
		EndRound();
	}
	else if(HumanCount && !InfectedCount && !m_InfectionTimer)
	{
		GameServer()->SendChatTarget(-1, "!| Less number of infecteds, start infection...");
		StartInfection();
	}
	else if(HumanCount && (g_Config.m_SvTimelimit > 0 && (Server()->Tick() - m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60))
	{
		GameServer()->SendChatTarget(-1, "!| Human...will never perish");
		EndRound();
	}
}

void CGameControllerInfectWar::StartRound()
{
	m_InfectionTimer = g_Config.m_SvInfectionTime * Server()->TickSpeed();

	mem_zero(m_aInfects, sizeof(m_aInfects));

	IGameController::StartRound();
}

void CGameControllerInfectWar::Tick()
{
	if(m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
	{
		if(m_InfectionTimer)
		{
			m_InfectionTimer--;

			if(!m_InfectionTimer)
			{
				GameServer()->SendChatTarget(-1, "!| Start infection...");
				StartInfection();
			}
		}

		for(auto& pPlayer : GameServer()->m_apPlayers)
		{
			if(!pPlayer)
				continue;

			if(m_aInfects[pPlayer->GetCID()])
			{
				pPlayer->m_TeeInfos.m_UseCustomColor = true;
				pPlayer->m_TeeInfos.m_ColorBody = 3866368;
				pPlayer->m_TeeInfos.m_ColorFeet = 65414;
			}
			else
			{
				pPlayer->m_TeeInfos.m_UseCustomColor = false;

				if(pPlayer->GetCharacter() && pPlayer->GetCharacter()->m_ActiveWeapon == WEAPON_GUN && pPlayer->GetCharacter()->m_Input.m_Fire&1)
				{
					m_aFixLasers[pPlayer->GetCID()] = true;

					float Distance = distance(vec2(pPlayer->GetCharacter()->m_LatestInput.m_TargetX, pPlayer->GetCharacter()->m_LatestInput.m_TargetY), vec2(0.f, 0.f));
					Distance = clamp(Distance, 32.0f, 240.0f);

					float Angle = GetAngle(vec2(pPlayer->GetCharacter()->m_LatestInput.m_TargetX, pPlayer->GetCharacter()->m_LatestInput.m_TargetY));
					m_aFixLaserPoints[pPlayer->GetCID()] = pPlayer->GetCharacter()->m_Pos + vec2(cos(Angle) * Distance, sin(Angle) * Distance);

					{
						vec2 At;
						CCharacter *pHit = GameServer()->m_World.IntersectCharacter(pPlayer->GetCharacter()->m_Pos, m_aFixLaserPoints[pPlayer->GetCID()] , 0.f, At, pPlayer->GetCharacter());
						if(pHit)
							m_aFixLaserPoints[pPlayer->GetCID()] = At;
					}

					if(Server()->Tick() % (Server()->TickSpeed() / 10) == 0)
					{
						pPlayer->GetCharacter()->m_AttackTick = Server()->Tick();
						if(Server()->Tick() % (Server()->TickSpeed() / 2) == 0)
						{
							GameServer()->CreateSound(pPlayer->GetCharacter()->m_Pos, SOUND_HOOK_LOOP);

							CCharacter *pHit = GameServer()->m_World.ClosestCharacter(m_aFixLaserPoints[pPlayer->GetCID()], CCharacter::ms_PhysSize / 2, pPlayer->GetCharacter());
							if(pHit)
								pHit->TakeDamage(vec2(0.f, 0.f), 1, pPlayer->GetCID(), WEAPON_GUN);
						}
					}
				}
				else
				{
					m_aFixLasers[pPlayer->GetCID()] = false;
				}
			}
		}
	}

	IGameController::Tick();
}
