#pragma once

#include <Core/World/ComponentManager.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

using ezCameraShakeComponentManager = ezComponentManagerSimple<class ezCameraShakeComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief This component is used to apply a shaking effect to the game object that it is attached to.
///
/// The shake is applied as a local rotation around the Y and Z axis, assuming the camera is looking along the positive X axis.
/// The component can be attached to the same object as a camera component,
/// but it is usually best to insert a dedicated shake object as a parent of the camera.
///
/// How much shake to apply is controlled through the m_MinShake and m_MaxShake properties.
///
/// The shake values can be modified dynamically to force a shake, but it is more convenient to instead place shake volumes (see ezCameraShakeVolumeComponent and derived classes). The camera shake component samples these volumes using its own location and uses the
/// determined strength to fade between its min and max shake amount.
///
/// \see ezCameraShakeVolumeComponent
class EZ_GAMECOMPONENTS_DLL ezCameraShakeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraShakeComponent, ezComponent, ezCameraShakeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraShakeComponent

  /// \brief How much shake to apply as the minimum value, even if no shake volume is found or the shake strength is zero.
  ezAngle m_MinShake; // [ property ]

  /// \brief How much shake to apply at shake strength 1.
  ezAngle m_MaxShake = ezAngle::MakeFromDegree(5); // [ property ]

public:
  ezCameraShakeComponent();
  ~ezCameraShakeComponent();

protected:
  void Update();

  void GenerateKeyframe();
  float GetStrengthAtPosition() const;

  float m_fLastStrength = 0.0f;
  ezTime m_ReferenceTime;
  ezAngle m_Rotation;
  ezQuat m_qPrevTarget = ezQuat::MakeIdentity();
  ezQuat m_qNextTarget = ezQuat::MakeIdentity();
};
