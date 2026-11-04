#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <Core/World/World.h>

class ProjectileComponent;
using ProjectileComponentManager = ezComponentManagerSimple<ProjectileComponent, ezComponentUpdateType::WhenSimulating>;

/// A bullet that flies in a straight line and wraps around the screen.
///
/// How long it lives is decided by the ezTimedDeathComponent in its prefab.
class ProjectileComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ProjectileComponent, ezComponent, ProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ProjectileComponent

public:
  ProjectileComponent();
  ~ProjectileComponent();

  void Update();

  float m_fSpeed = 70.0f; // [ property ] speed relative to the ship
  float m_fRadius = 0.3f; // [ property ] the collision radius

  /// Set by the ship when it fires.
  ezVec3 m_vVelocity = ezVec3::MakeZero();
};
