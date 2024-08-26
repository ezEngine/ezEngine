#pragma once

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgComponentInternalTrigger;
struct ezMsgDeleteGameObject;

/// \brief Base class for components that define volumes in which a camera shake effect shall be applied.
///
/// Derived classes implement different shape types and how the shake strength is calculated.
class EZ_GAMECOMPONENTS_DLL ezCameraShakeVolumeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezCameraShakeVolumeComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraShakeVolumeComponent

public:
  ezCameraShakeVolumeComponent();
  ~ezCameraShakeVolumeComponent();

  /// \brief The spatial category used to find camera shake volume components through the spatial system.
  static ezSpatialData::Category SpatialDataCategory;

  /// \brief How long a shake burst should last. Zero for constant shaking.
  ezTime m_BurstDuration; // [ property ]

  /// \brief How strong the shake should be at the strongest point. Typically a value between one and zero.
  float m_fStrength; // [ property ]

  /// \brief Calculates the shake strength at the given global position.
  float ComputeForceAtGlobalPosition(const ezSimdVec4f& vGlobalPos) const;

  /// \brief Calculates the shake strength in local space of the component.
  virtual float ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const = 0;

  /// \brief In case of a burst shake, defines whether the component should delete itself afterwards.
  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(ezMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using ezCameraShakeVolumeSphereComponentManager = ezComponentManager<class ezCameraShakeVolumeSphereComponent, ezBlockStorageType::Compact>;

/// \brief A spherical volume in which a camera shake will be applied.
///
/// The shake strength is strongest at the center of the sphere and gradually weaker towards the sphere radius.
///
/// \see ezCameraShakeVolumeComponent
/// \see ezCameraShakeComponent
class EZ_GAMECOMPONENTS_DLL ezCameraShakeVolumeSphereComponent : public ezCameraShakeVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraShakeVolumeSphereComponent, ezCameraShakeVolumeComponent, ezCameraShakeVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraShakeVolumeSphereComponent

public:
  ezCameraShakeVolumeSphereComponent();
  ~ezCameraShakeVolumeSphereComponent();

  virtual float ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  ezSimdFloat m_fOneDivRadius;
};
