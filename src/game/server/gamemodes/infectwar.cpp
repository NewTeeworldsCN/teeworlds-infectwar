#include <game/server/gamecontext.h>
#include "infectwar.h"

CGameControllerInfectWar::CGameControllerInfectWar(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "InfectWar fng";
}

void CGameControllerInfectWar::Tick()
{
	IGameController::Tick();
}
