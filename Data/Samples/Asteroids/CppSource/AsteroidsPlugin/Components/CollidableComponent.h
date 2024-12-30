#pragma once

#include <Core/World/World.h>

class CollidableComponent;
using CollidableComponentManager = ezComponentManager<CollidableComponent, ezBlockStorageType::FreeList>;

class CollidableComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(CollidableComponent, ezComponent, CollidableComponentManager);

public:
  CollidableComponent();

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override {}

  float m_fCollisionRadius;
};
