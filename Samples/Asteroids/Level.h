#pragma once

#include <Core/World/World.h>
#include <Core/Graphics/Camera.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

typedef ezTypedResourceHandle<class ezCollectionResource> ezCollectionResourceHandle;

#define MaxPlayers 4
#define MaxAsteroids 30
#define MaxPlayerActions 7

class Level
{
public:
  Level();

  void SetupLevel(ezWorld* pWorld);
  void UpdatePlayerInput(ezInt32 iPlayer);

  ezWorld* GetWorld() const { return m_pWorld; }
  const ezCamera* GetCamera() const { return &m_Camera; }

private:
  void CreatePlayerShip(ezInt32 iPlayer);
  void CreateAsteroid();

  ezCollectionResourceHandle m_hAssetCollection;
  ezWorld* m_pWorld;
  ezGameObjectHandle m_hPlayerShips[MaxPlayers];
  ezCamera m_Camera;
};

