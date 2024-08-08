#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>

#include "laser.h"
#include "pickup.h"
#include "projectile.h"
#include "turret.h"

static const float ms_SearchDistance = 480.0f;

CTurret::CTurret(CGameWorld *pGameWorld, vec2 Pos, bool Attacker, int Weapon, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_TURRET)
{
	m_Pos = Pos;
	m_ProximityRadius = TurretPhysSize;

	m_Attacker = Attacker;
	m_Owner = Owner;
	m_Weapon = Weapon;
	m_DragPos = Pos;

	if(Attacker)
		m_ReloadTimer = 0;
	else
		m_ReloadTimer = Server()->TickSpeed() * (Weapon + 1) * 5;
	m_StartTick = Server()->Tick();

	m_TouchNum = 0;

	for(int i = 0; i < TURRET_IDCOUNT; i++)
	{
		m_IDs[i] = Server()->SnapNewID();
	}

	GameWorld()->InsertEntity(this);
}

CTurret::~CTurret()
{
	for(int i = 0; i < TURRET_IDCOUNT; i++)
	{
		Server()->SnapFreeID(m_IDs[i]);
	}
}

void CTurret::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CTurret::Tick()
{
	m_DragPos = m_Pos;
	if(m_ReloadTimer)
		m_ReloadTimer--;

	if(m_Owner != -2)
	{
		if(!GameServer()->GetPlayerChar(m_Owner))
		{	
			Reset();
			return;
		}
	}

	if(m_Attacker)
	{
		CCharacter *apEnts[MAX_CLIENTS];
		int Num = GameWorld()->FindEntities(m_Pos, ms_SearchDistance, (CEntity**) apEnts,
													MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

		float ClosetDistance = ms_SearchDistance * 2;
		CCharacter *pCloset = nullptr;
		for (int i = 0; i < Num; ++i)
		{
			CCharacter *pChr = apEnts[i];
			if(GameServer()->m_pController->IsFriendlyFire(m_Owner, pChr->GetPlayer()->GetCID()) || GameServer()->Collision()->IntersectLine(m_Pos, pChr->m_Pos, nullptr, nullptr))
				continue;
			float Distance = distance(pChr->m_Pos, m_Pos);
			if(Distance < ClosetDistance + pChr->m_ProximityRadius)
			{
				ClosetDistance = Distance;
				pCloset = pChr;
				if(ClosetDistance < m_ProximityRadius)
				{
					pChr->TakeDamage(vec2(0.f, -1.f), 15, m_Owner, m_Weapon);
					if(m_Weapon == WEAPON_NINJA)
					{
						m_TouchNum++;
						if(m_TouchNum < 5)
							continue;
					}
					GameWorld()->DestroyEntity(this);
					GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
					new CDroppedPickup(&GameServer()->m_World, 
						random_int(0, 1) ? POWERUP_ARMOR : POWERUP_HEALTH,
						0,
						true);
					return;
				}
			}
		}

		if(m_ReloadTimer)
			return;

		if(pCloset)
		{
			if(m_Weapon == WEAPON_HAMMER)
			{
				CCharacter *apEnts[MAX_CLIENTS];
				int Hits = 0;
				int Num = GameServer()->m_World.FindEntities(m_Pos, pCloset->m_ProximityRadius * 4.5f, (CEntity**)apEnts,
															MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

				for (int i = 0; i < Num; ++i)
				{
					CCharacter *pTarget = apEnts[i];

					if(GameServer()->m_pController->IsFriendlyFire(m_Owner, pTarget->GetPlayer()->GetCID()) || GameServer()->Collision()->IntersectLine(m_Pos, pTarget->m_Pos, nullptr, nullptr))
						continue;

					// set his velocity to fast upward (for now)
					if(length(pTarget->m_Pos - m_Pos) > 0.0f)
						GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - m_Pos) * pTarget->m_ProximityRadius * 0.5f);
					else
						GameServer()->CreateHammerHit(m_Pos);

					vec2 Dir;
					if(length(pTarget->m_Pos - m_Pos) > 0.0f)
						Dir = normalize(pTarget->m_Pos - m_Pos);
					else
						Dir = vec2(0.f, -1.f);

					pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
						m_Owner, WEAPON_HAMMER);
					Hits++;
				}
				// if we Hit anything, we have to wait for the reload
				if(Hits)
					m_ReloadTimer = Server()->TickSpeed()/3*2;
				if(!m_ReloadTimer)
					m_ReloadTimer = g_pData->m_Weapons.m_aId[m_Weapon].m_Firedelay * Server()->TickSpeed() / 1000;
			}
			else if(m_Weapon == WEAPON_NINJA)
			{
				m_DragPos = pCloset->m_Pos;
				if(ClosetDistance > pCloset->m_ProximityRadius * 1.50f) // TODO: fix tweakable variable
				{
					float Accel = GameWorld()->m_Core.m_Tuning.m_HookDragAccel * (ClosetDistance / GameWorld()->m_Core.m_Tuning.m_HookLength);
					float DragSpeed = GameWorld()->m_Core.m_Tuning.m_HookDragSpeed;

					vec2 Dir = normalize(m_Pos - pCloset->m_Pos);
					// add force to the dragged player
					pCloset->Core()->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCloset->Core()->m_Vel.x, Accel * Dir.x * 1.5f);
					pCloset->Core()->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCloset->Core()->m_Vel.y, Accel * Dir.y * 1.5f);
				}
			}
			else
			{
				vec2 Direction = normalize(pCloset->m_Pos - m_Pos);
				switch (m_Weapon)
				{
					case WEAPON_GUN:
					{
						new CProjectile(&GameServer()->m_World, WEAPON_GUN,
							m_Owner,
							m_Pos,
							Direction,
							(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
							1, 0, 0, -1, WEAPON_GUN);

						GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
					}
					break;

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
								m_Owner,
								m_Pos,
								vec2(cosf(a), sinf(a)) * Speed,
								(int)(Server()->TickSpeed() * GameServer()->Tuning()->m_ShotgunLifetime),
								1, 0, 0, -1, WEAPON_SHOTGUN);
						}

						GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
					}
					break;

					case WEAPON_GRENADE:
					{
						new CProjectile(&GameServer()->m_World, WEAPON_GRENADE,
							m_Owner,
							m_Pos,
							Direction,
							(int) (Server()->TickSpeed() * GameServer()->Tuning()->m_GrenadeLifetime),
							1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

						GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
					}
					break;

					case WEAPON_RIFLE:
					{
						new CLaser(&GameServer()->m_World, m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_Owner);
						GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
					}
					break;
				}

				if(!m_ReloadTimer)
					m_ReloadTimer = g_pData->m_Weapons.m_aId[m_Weapon].m_Firedelay * 2.0f * Server()->TickSpeed() / 1000;
			}
		}
	}
	else if(!m_ReloadTimer)
	{
		float Spreading[] = {-2.875f, -1.425f, 0.f, 1.425f, 2.875f};
		float Angle = GetAngle(vec2(0.f, -1.f)) + Spreading[random_int(0, 4)];
		CDroppedPickup *pPickup = nullptr;
		if(m_Weapon == WEAPON_HAMMER)
		{
			pPickup = new CDroppedPickup(&GameServer()->m_World, 
				random_int(0, 1) ? POWERUP_ARMOR : POWERUP_HEALTH,
				0,
				true);
		}
		else
		{
			pPickup = new CDroppedPickup(&GameServer()->m_World, 
				m_Weapon == WEAPON_NINJA ? POWERUP_NINJA : POWERUP_WEAPON,
				m_Weapon == WEAPON_NINJA ? 0 : m_Weapon,
				true);
		}
		pPickup->SetVel(GetDir(Angle) * 8.0f);
		pPickup->m_Pos = m_Pos;

		m_ReloadTimer = Server()->TickSpeed() * (m_Weapon + 1) * 5;
	}
}

void CTurret::TickPaused()
{
	if(m_StartTick != -1)
		++m_StartTick;
}

void CTurret::Snap(int SnappingClient)
{
	if(m_Weapon == WEAPON_HAMMER)
	{
		CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
		if(!pPickup)
			return;

		pPickup->m_X = (int) m_Pos.x;
		pPickup->m_Y = (int) m_Pos.y;
		pPickup->m_Type = POWERUP_ARMOR;
		pPickup->m_Subtype = 0;
	}
	else if(m_Weapon == WEAPON_NINJA)
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
		if(!pObj)
			return;

		pObj->m_X = (int) m_DragPos.x;
		pObj->m_Y = (int) m_DragPos.y;
		pObj->m_FromX = (int) m_Pos.x;
		pObj->m_FromY = (int) m_Pos.y;
		pObj->m_StartTick = (int) (Server()->Tick() / 15) * 15;
	}
	else
	{
		CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
		if(!pProj)
			return;

        pProj->m_X = (int) m_Pos.x;
        pProj->m_Y = (int) m_Pos.y;
		pProj->m_VelX = 0;
		pProj->m_VelY = 0;
		pProj->m_Type = m_Weapon;
		pProj->m_StartTick = Server()->Tick() - 1;
	}

    // infclass
    float Time = (Server()->Tick() - m_StartTick) / (float) Server()->TickSpeed();
    float Angle = fmodf(Time * pi / 2.0f, 2.0f * pi);
    for(int i = 0; i < TURRET_IDCOUNT; i ++)
    {
        float FromShiftedAngle = Angle + 2.0 * pi * static_cast<float>(i) / static_cast<float>(TURRET_IDCOUNT);
        float ToShiftedAngle = Angle + 2.0 * pi * static_cast<float>((i + 1) % TURRET_IDCOUNT) / static_cast<float>(TURRET_IDCOUNT);

		vec2 From = vec2(m_Pos.x + 32.0f * cos(FromShiftedAngle), m_Pos.y + 32.0f * sin(FromShiftedAngle));
		vec2 To = vec2(m_Pos.x + 32.0f * cos(ToShiftedAngle), m_Pos.y + 32.0f * sin(ToShiftedAngle));

		vec2 Direction = normalize(To - From);

        CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_IDs[i], sizeof(CNetObj_Projectile)));
		if(!pProj)
			return;

        pProj->m_X = (int) From.x;
        pProj->m_Y = (int) From.y;
		pProj->m_VelX = (int) (Direction.x * 100.0f);
		pProj->m_VelY = (int) (Direction.y * 100.0f);
		pProj->m_Type = m_Attacker ? WEAPON_HAMMER : WEAPON_RIFLE;
		pProj->m_StartTick = Server()->Tick();
    }
}
