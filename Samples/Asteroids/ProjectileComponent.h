#pragma once

#include <Core/World/World.h>

class ProjectileComponent;
typedef ezComponentManagerSimple<ProjectileComponent> ProjectileComponentManager;

class ProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ProjectileComponent, ProjectileComponentManager);

public:
  ProjectileComponent();
  void Update();

  ezInt32 m_iTimeToLive;
  ezVec3 m_vVelocity;
  ezVec3 m_vDrawDir;
  ezInt32 m_iBelongsToPlayer;
  bool m_bDoesDamage;
};


