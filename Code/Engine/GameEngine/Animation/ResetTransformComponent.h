#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using ezResetTransformComponentManager = ezComponentManager<class ezResetTransformComponent, ezBlockStorageType::Compact>;

/// \brief This component sets the local transform of its owner to known values when the simulation starts.
///
/// This component is meant for use cases where an object may be activated and deactivated over and over.
/// For example due to a state machine switching between different objects states by (de-)activating a sub-tree of objects.
///
/// Every time an object becomes active, it may want to start moving again from fixed location.
/// This component helps with that, by reseting the local transform of its owner to such a fixed location once.
///
/// After that, it does nothing else.
class EZ_GAMEENGINE_DLL ezResetTransformComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezResetTransformComponent, ezComponent, ezResetTransformComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezTransformComponent

public:
  ezResetTransformComponent();
  ~ezResetTransformComponent();

  ezVec3 m_vLocalPosition = ezVec3::MakeZero();
  ezQuat m_qLocalRotation = ezQuat::MakeIdentity();
  ezVec3 m_vLocalScaling = ezVec3(1, 1, 1);
  float m_fLocalUniformScaling = 1.0f;

  bool m_bResetLocalPositionX = true;
  bool m_bResetLocalPositionY = true;
  bool m_bResetLocalPositionZ = true;
  bool m_bResetLocalRotation = true;
  bool m_bResetLocalScaling = true;
};
