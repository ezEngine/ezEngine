#pragma once

#include <Core/World/World.h>

class ProjectileComponent;
typedef ezComponentManagerSimple<ProjectileComponent, ezComponentUpdateType::WhenSimulating> ProjectileComponentManager;

class ProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ProjectileComponent, ezComponent, ProjectileComponentManager);

public:
  ProjectileComponent();
  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& stream) override {}

  ezTime m_TimeToLive;
  float m_fSpeed;
  ezInt32 m_iBelongsToPlayer;
  bool m_bDoesDamage;
};


