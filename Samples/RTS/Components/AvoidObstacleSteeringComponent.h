#pragma once

#include <Core/World/World.h>
#include <Core/World/GameObject.h>
#include <RTS/Components/SteeringBehaviorComponent.h>
#include <GameUtils/GridAlgorithms/Rasterization.h>

class AvoidObstacleSteeringComponent;
typedef ezComponentManagerSimple<AvoidObstacleSteeringComponent> AvoidObstacleSteeringComponentManager;

class AvoidObstacleSteeringComponent : public SteeringBehaviorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(AvoidObstacleSteeringComponent, AvoidObstacleSteeringComponentManager);

public:
  AvoidObstacleSteeringComponent();


private:
  virtual void Update();

  static ezCallbackResult::Enum ComputeCellDanger(ezInt32 x, ezInt32 y, void* pPassThrough);
};

