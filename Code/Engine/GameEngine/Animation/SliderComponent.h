#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Animation/TransformComponent.h>

using ezSliderComponentManager = ezComponentManagerSimple<class ezSliderComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Applies a sliding transform to the game object that it is attached to.
///
/// The object is moved along a local axis either once or back and forth.
class EZ_GAMEENGINE_DLL ezSliderComponent : public ezTransformComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSliderComponent, ezTransformComponent, ezSliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSliderComponent

public:
  ezSliderComponent();
  ~ezSliderComponent();

  /// \brief How far to move the object along the axis before reaching the end point.
  float m_fDistanceToTravel = 1.0f; // [ property ]

  /// \brief The acceleration to use to reach the target speed.
  float m_fAcceleration = 0.0f; // [ property ]

  /// \brief The deceleration to use to brake to zero speed before reaching the end.
  float m_fDeceleration = 0.0; // [ property ]

  /// \brief The axis along which to move the object.
  ezEnum<ezBasisAxis> m_Axis = ezBasisAxis::PositiveZ; // [ property ]

  /// \brief If non-zero, the slider starts at a random offset as if it had already been moving for up to this amount of time.
  ezTime m_RandomStart; // [ property ]

protected:
  void Update();

  float m_fLastDistance = 0.0f;
};
