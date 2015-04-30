#pragma once

#include <Core/World/World.h>

class CollidableComponent;
typedef ezComponentManager<CollidableComponent> CollidableComponentManager;

class CollidableComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(CollidableComponent, CollidableComponentManager);

public:
  CollidableComponent();

  float m_fCollisionRadius;
};


