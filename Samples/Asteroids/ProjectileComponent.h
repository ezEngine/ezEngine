#pragma once

#include <Core/World/World.h>

class ProjectileComponent;
typedef ezComponentManagerSimple<ProjectileComponent, true> ProjectileComponentManager;

class ProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ProjectileComponent, ezComponent, ProjectileComponentManager);

public:
  ProjectileComponent();
  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override {}
  virtual void DeserializeComponent(ezWorldReader& stream, ezUInt32 uiTypeVersion) override {}

  ezInt32 m_iTimeToLive;
  ezVec3 m_vVelocity;
  ezVec3 m_vDrawDir;
  ezInt32 m_iBelongsToPlayer;
  bool m_bDoesDamage;
};


