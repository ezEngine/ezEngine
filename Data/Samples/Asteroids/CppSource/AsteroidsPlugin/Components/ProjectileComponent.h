#pragma once

#include <Core/World/World.h>

class ProjectileComponent;
using ProjectileComponentManager = ezComponentManagerSimple<ProjectileComponent, ezComponentUpdateType::WhenSimulating>;

class ProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ProjectileComponent, ezComponent, ProjectileComponentManager);

public:
  ProjectileComponent();
  void Update();

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override {}

  ezTime m_TimeToLive;
  float m_fSpeed = 0.0f;
  ezInt32 m_iBelongsToPlayer = -1;
  float m_fDoesDamage = 0.0f;
};
