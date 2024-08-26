#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using ezRotorComponentManager = ezComponentManagerSimple<class ezRotorComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Applies a rotation to the game object that it is attached to.
///
/// The rotation may be endless, or limited to a certain amount of rotation.
/// It may also automatically turn around and accelerate and decelerate.
class EZ_GAMEENGINE_DLL ezRotorComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRotorComponent, ezTransformComponent, ezRotorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRotorComponent

public:
  ezRotorComponent();
  ~ezRotorComponent();

  /// \brief How much to rotate before reaching the end and either stopping or turning around.
  /// Set to zero for endless rotation.
  ezInt32 m_iDegreeToRotate = 0; // [ property ]

  /// \brief The acceleration to reach the target speed.
  float m_fAcceleration = 1.0f; // [ property ]

  /// \brief The deceleration to brake to zero speed before reaching the end rotation.
  float m_fDeceleration = 1.0f; // [ property ]

  /// \brief The axis around which to rotate. In local space of the game object.
  ezEnum<ezBasisAxis> m_Axis = ezBasisAxis::PositiveZ; // [ property ]

  /// \brief How much the rotation axis may randomly deviate to not have all objects rotate the same way.
  ezAngle m_AxisDeviation; // [ property ]

protected:
  void Update();

  ezVec3 m_vRotationAxis = ezVec3(0, 0, 1);
  ezQuat m_qLastRotation = ezQuat::MakeIdentity();
};
