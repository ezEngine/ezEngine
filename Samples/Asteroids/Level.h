#pragma once

#include <Core/World/World.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

#define MaxPlayers 4
#define MaxAsteroids 30
#define MaxPlayerActions 7

class Level
{
public:
  Level();
  ~Level();

  void SetupLevel(ezWorld* pWorld);

  ezWorld* GetWorld() const { return m_pWorld; }
  const ezCamera* GetCamera() const { return &m_Camera; }

private:
  void UpdatePlayerInput(ezInt32 iPlayer);
  void CreatePlayerShip(ezInt32 iPlayer);
  void CreateAsteroid();

  ezMeshResourceHandle m_hAsteroidMesh;
  ezMaterialResourceHandle m_hAsteroidMaterial;
  ezWorld* m_pWorld;
  ezGameObjectHandle m_hPlayerShips[MaxPlayers];
  ezCamera m_Camera;
};

