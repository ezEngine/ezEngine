#pragma once

#include <Core/World/World.h>

class CollidableComponent;
typedef ezComponentManager<CollidableComponent> CollidableComponentManager;

class CollidableComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(CollidableComponent, ezComponent, CollidableComponentManager);

public:
  CollidableComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& stream) override {}

  float m_fCollisionRadius;
};


