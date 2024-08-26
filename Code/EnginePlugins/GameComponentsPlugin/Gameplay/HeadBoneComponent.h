#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

using ezHeadBoneComponentManager = ezComponentManagerSimple<class ezHeadBoneComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Applies a vertical rotation in local space (local Y axis) to the owner game object.
///
/// This component is meant to be used to apply a vertical rotation to a camera.
/// For first-person camera movement, typically the horizontal rotation is already taken care of
/// through the rotation of a character controller.
/// To additionally allow a limited vertical rotation, this component is introduced.
/// It is assumed that a local rotation of zero represents the "forward" camera direction and the camera is allowed
/// to rotate both up and down by a certain number of degrees, for example 80 degrees.
/// This component takes care to apply that amount of rotation and not more.
///
/// Call SetVerticalRotation() or ChangeVerticalRotation() to set or add some rotation.
class EZ_GAMECOMPONENTS_DLL ezHeadBoneComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezHeadBoneComponent, ezComponent, ezHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezHeadBoneComponent

public:
  ezHeadBoneComponent();
  ~ezHeadBoneComponent();

  /// \brief Sets the vertical rotation to a fixed value.
  ///
  /// The final rotation will be clamped to the maximum allowed value.
  void SetVerticalRotation(float fRadians); // [ scriptable ]

  /// \brief Adds or subtracts from the current rotation.
  ///
  /// The final rotation will be clamped to the maximum allowed value.
  void ChangeVerticalRotation(float fRadians);                 // [ scriptable ]

  ezAngle m_MaxVerticalRotation = ezAngle::MakeFromDegree(80); // [ property ]

protected:
  void Update();

  ezAngle m_NewVerticalRotation;
  ezAngle m_CurVerticalRotation;
};
