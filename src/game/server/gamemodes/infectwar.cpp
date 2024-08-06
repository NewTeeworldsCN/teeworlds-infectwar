#include <game/server/gamecontext.h>
#include "infectwar.h"

CGameControllerInfectWar::CGameControllerInfectWar(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "InfectWar Catch";
}

void CGameControllerInfectWar::Tick()
{
	IGameController::Tick();
}
