#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <Core/World/World.h>

class PickupComponent;
using PickupComponentManager = ezComponentManagerSimple<PickupComponent, ezComponentUpdateType::WhenSimulating>;

struct PickupType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Shield,     ///< Absorbs one hit.
    TripleShot, ///< Fires three projectiles at once for a while.

    Default = Shield
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, PickupType);

/// A power-up that is sometimes left behind by a destroyed asteroid.
///
/// It drifts until the ship collects it (detected by AsteroidsGameComponent) or until its time runs out.
class PickupComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(PickupComponent, ezComponent, PickupComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // PickupComponent

public:
  PickupComponent();
  ~PickupComponent();

  void Update();

  ezEnum<PickupType> m_Type;                       // [ property ]
  ezString m_sCollectSound;                        // [ property ]
  float m_fRadius = 1.8f;                          // [ property ] the collision radius
  ezTime m_LifeTime = ezTime::MakeFromSeconds(10); // [ property ] counts down while the pickup is in the game

  ezVec3 m_vVelocity = ezVec3::MakeZero();
};
