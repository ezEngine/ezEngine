#pragma once

#include "Main.h"
#include <Core/World/World.h>

class Level
{
public:
  Level();
  ~Level();

  void SetupLevel(const char* szLevelName);
  void Update();

  const ezWorld* GetWorld() const { return m_pWorld; }

private:
  void UpdatePlayerInput(ezInt32 iPlayer);
  void CreatePlayerShip(ezInt32 iPlayer);
  void CreateAsteroid();

  ezWorld* m_pWorld;
  ezGameObjectHandle m_hPlayerShips[MaxPlayers];
};

