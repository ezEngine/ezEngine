#pragma once

#include <AsteroidsPlugin/AsteroidsPluginDLL.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>

class AsteroidComponent;
using AsteroidComponentManager = ezComponentManagerSimple<AsteroidComponent, ezComponentUpdateType::WhenSimulating>;

/// A drifting and tumbling rock. Splitting it up and scoring is handled by AsteroidsGameComponent.
class AsteroidComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(AsteroidComponent, ezComponent, AsteroidComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // AsteroidComponent

public:
  AsteroidComponent();
  ~AsteroidComponent();

  void Update();

  /// Sets up size and movement.
  ///
  /// Size 0 is the largest and the one a wave starts with. A hit splits an asteroid into two of the next size.
  /// The prefab's meshes are expected to have a radius of about 1, the object gets scaled by fRadius.
  void Launch(ezUInt8 uiSize, float fRadius, const ezVec3& vVelocity);

  float GetRadius() const { return m_fRadius; }
  ezUInt8 GetSize() const { return m_uiSize; }
  const ezVec3& GetVelocity() const { return m_vVelocity; }

  ezPrefabResourceHandle m_hExplosionPrefab; // [ property ] receives ezMsgSetFloatParameter 'Scale' when spawned

  /// Set when the asteroid got hit, so that it can't be hit a second time in the same frame.
  bool m_bDestroyed = false;

private:
  ezVec3 m_vVelocity = ezVec3::MakeZero();
  ezVec3 m_vSpinAxis = ezVec3(0, 0, 1);
  ezAngle m_SpinSpeed;
  float m_fRadius = 1.0f;
  ezUInt8 m_uiSize = 0;
};
