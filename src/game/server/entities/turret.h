#ifndef GAME_SERVER_ENTITIES_TURRET_H
#define GAME_SERVER_ENTITIES_TURRET_H

#include <game/server/entity.h>

#define TURRET_IDCOUNT 3
const int TurretPhysSize = 24;

class CTurret : public CEntity
{
public:
	CTurret(CGameWorld *pGameWorld, vec2 Pos, bool Attacker, int Weapon, int Owner);
	~CTurret();

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;

public:
	bool m_Attacker;
	int m_Owner;
	int m_Weapon;
	int m_IDs[TURRET_IDCOUNT];
	int m_ReloadTimer;
	int m_TouchNum;
	int64_t m_StartTick;
	vec2 m_DragPos;
};

#endif // GAME_SERVER_ENTITIES_TURRET_H
