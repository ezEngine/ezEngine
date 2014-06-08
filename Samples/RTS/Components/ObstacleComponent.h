#pragma once

#include <Core/World/World.h>
#include <Core/World/GameObject.h>
#include <GameUtils/DataStructures/GameGrid.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>

class ObstacleComponent;
typedef ezComponentManager<ObstacleComponent> ObstacleComponentManager;

class ObstacleComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ObstacleComponent, ObstacleComponentManager);

public:
  ObstacleComponent();

  static float g_fDefaultRadius;
  float m_fRadius;

};

