#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgComponentInternalTrigger;
struct ezMsgDeleteGameObject;

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

  static ezSpatialData::Category SpatialDataCategory;

  ezTime m_BurstDuration;            // [ property ]
  ezEnum<ezWindStrength> m_Strength; // [ property ]
  bool m_bReverseDirection = false;  // [ property ]

  ezSimdVec4f ComputeForceAtGlobalPosition(const ezSimdVec4f& vGlobalPos) const;

  virtual ezSimdVec4f ComputeForceAtLocalPosition(const ezSimdVec4f& vLocalPos) const = 0;

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

struct ezWindVolumeCylinderMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Directional,
    Vortex,

    Default = Directional
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezWindVolumeCylinderMode);

using ezWindVolumeCylinderComponentManager = ezComponentManager<class ezWindVolumeCylinderComponent, ezBlockStorageType::Compact>;

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

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

  float GetLength() const { return m_fLength; } // [ property ]
  void SetLength(float fVal);                   // [ property ]

  ezEnum<ezWindVolumeCylinderMode> m_Mode; // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  float m_fLength = 5.0f;
  ezSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using ezWindVolumeConeComponentManager = ezComponentManager<class ezWindVolumeConeComponent, ezBlockStorageType::Compact>;

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

  ezAngle GetAngle() const { return m_Angle; } // [ property ]
  void SetAngle(ezAngle val);                  // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  float m_fLength = 1.0f;
  ezAngle m_Angle = ezAngle::Degree(45);
};
