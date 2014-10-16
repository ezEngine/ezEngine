#pragma once

#include "Main.h"
#include <Core/World/World.h>
#include <CoreUtils/Graphics/Camera.h>

class Level
{
public:
  Level();
  ~Level();

  void SetupLevel(const char* szLevelName);
  void Update();

  ezWorld* GetWorld() const { return m_pWorld; }
  const ezCamera* GetCamera() const { return &m_Camera; }

private:
  void UpdatePlayerInput(ezInt32 iPlayer);
  void CreatePlayerShip(ezInt32 iPlayer);
  void CreateAsteroid();

  ezWorld* m_pWorld;
  ezGameObjectHandle m_hPlayerShips[MaxPlayers];
  ezCamera m_Camera;
};

