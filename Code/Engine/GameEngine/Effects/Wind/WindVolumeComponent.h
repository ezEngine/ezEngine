#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgComponentInternalTrigger;
struct ezMsgDeleteGameObject;

/// \brief Base class for components that define wind volumes.
///
/// These components define the shape in which to apply wind to objects that support this functionality.
class EZ_GAMEENGINE_DLL ezWindVolumeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezWindVolumeComponent, ezComponent);

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
  // ezWindVolumeComponent

public:
  ezWindVolumeComponent();
  ~ezWindVolumeComponent();

  /// \brief The spatial category to use to find all wind volume components through the spatial system.
  static ezSpatialData::Category SpatialDataCategory;

  /// \brief If non-zero, the wind will only last for a limited amount of time.
  ezTime m_BurstDuration; // [ property ]

  /// \brief How strong the wind shall blow at the strongest point of the volume.
  ezEnum<ezWindStrength> m_Strength; // [ property ]

  /// \brief Factor to scale the wind strength. Negative values can be used to reverse the wind direction.
  float m_fStrengthFactor = 1.0f;

  /// \brief Computes the wind force at a global position.
  ///
  /// Only the x,y,z components are used, they are a wind direction vector scaled to the wind speed.
  ezSimdVec4f ComputeForceAtGlobalPosition(const ezSimdVec4f& vGlobalPos) const;

  virtual ezSimdVec4f ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const = 0;

  /// \brief What happens after the wind burst is over.
  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(ezMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);

  float GetWindInMetersPerSecond() const;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using ezWindVolumeSphereComponentManager = ezComponentManager<class ezWindVolumeSphereComponent, ezBlockStorageType::Compact>;

/// \brief A spherical shape in which wind shall be applied to objects.
///
/// The wind blows outwards from the center of the sphere. If the wind direction is reversed, it pulls objects inwards.
class EZ_GAMEENGINE_DLL ezWindVolumeSphereComponent : public ezWindVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezWindVolumeSphereComponent, ezWindVolumeComponent, ezWindVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezWindVolumeSphereComponent

public:
  ezWindVolumeSphereComponent();
  ~ezWindVolumeSphereComponent();

  virtual ezSimdVec4f ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  ezSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief How the wind direction shall be calculated in a cylindrical wind volume.
struct ezWindVolumeCylinderMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Directional, ///< The wind direction is outwards from the cylinder.
    Vortex,      ///< The wind direction is tangential, moving in a circular fashion around the cylinder like in a tornado.

    Default = Directional
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezWindVolumeCylinderMode);

using ezWindVolumeCylinderComponentManager = ezComponentManager<class ezWindVolumeCylinderComponent, ezBlockStorageType::Compact>;

/// \brief A cylindrical volume in which wind shall be applied.
///
/// The wind direction may be either outwards from the cylinder center, or tangential (a vortex).
class EZ_GAMEENGINE_DLL ezWindVolumeCylinderComponent : public ezWindVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezWindVolumeCylinderComponent, ezWindVolumeComponent, ezWindVolumeCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezWindVolumeCylinderComponent

public:
  ezWindVolumeCylinderComponent();
  ~ezWindVolumeCylinderComponent();

  virtual ezSimdVec4f ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; }                   // [ property ]
  void SetRadius(float fVal);                                     // [ property ]

  float GetRadiusFalloff() const { return m_fRadiusFalloff; }     // [ property ]
  void SetRadiusFalloff(float fVal);                              // [ property ]

  float GetLength() const { return m_fLength; }                   // [ property ]
  void SetLength(float fVal);                                     // [ property ]

  float GetPositiveFalloff() const { return m_fPositiveFalloff; } // [ property ]
  void SetPositiveFalloff(float fVal);                            // [ property ]

  float GetNegativeFalloff() const { return m_fNegativeFalloff; } // [ property ]
  void SetNegativeFalloff(float fVal);                            // [ property ]

  ezEnum<ezWindVolumeCylinderMode> m_Mode;                        // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  void ComputeScaleBiasValues();

  float m_fRadius = 1.0f;
  float m_fRadiusFalloff = 0.0f;
  float m_fLength = 5.0f;
  float m_fPositiveFalloff = 0.0f;
  float m_fNegativeFalloff = 0.0f;

  ezSimdVec4f m_vScaleValues;
  ezSimdVec4f m_vBiasValues;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using ezWindVolumeConeComponentManager = ezComponentManager<class ezWindVolumeConeComponent, ezBlockStorageType::Compact>;

/// \brief A conical shape in which wind shall be applied to objects.
///
/// The wind is applied from the tip of the cone along the cone axis.
/// Strength falloff is only by distance along the cone main axis.
class EZ_GAMEENGINE_DLL ezWindVolumeConeComponent : public ezWindVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezWindVolumeConeComponent, ezWindVolumeComponent, ezWindVolumeConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezWindVolumeCylinderComponent

public:
  ezWindVolumeConeComponent();
  ~ezWindVolumeConeComponent();

  virtual ezSimdVec4f ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const override;

  float GetLength() const { return m_fLength; } // [ property ]
  void SetLength(float fVal);                   // [ property ]

  ezAngle GetAngle() const { return m_Angle; }  // [ property ]
  void SetAngle(ezAngle val);                   // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  float m_fLength = 1.0f;
  ezAngle m_Angle = ezAngle::MakeFromDegree(45);
};
