#pragma once

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgUpdateLocalBounds;
struct ezMsgComponentInternalTrigger;
struct ezMsgDeleteGameObject;

class EZ_GAMEENGINE_DLL ezCameraShakeVolumeComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezCameraShakeVolumeComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraShakeVolumeComponent

public:
  ezCameraShakeVolumeComponent();
  ~ezCameraShakeVolumeComponent();

  static ezSpatialData::Category SpatialDataCategory;

  ezTime m_BurstDuration; // [ property ]
  float m_fStrength;      // [ property ]

  float ComputeForceAtGlobalPosition(const ezSimdVec4f& globalPos) const;

  virtual float ComputeForceAtLocalPosition(const ezSimdVec4f& localPos) const = 0;

  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(ezMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using ezCameraShakeVolumeSphereComponentManager = ezComponentManager<class ezCameraShakeVolumeSphereComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezCameraShakeVolumeSphereComponent : public ezCameraShakeVolumeComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraShakeVolumeSphereComponent, ezCameraShakeVolumeComponent, ezCameraShakeVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCameraShakeVolumeSphereComponent

public:
  ezCameraShakeVolumeSphereComponent();
  ~ezCameraShakeVolumeSphereComponent();

  virtual float ComputeForceAtLocalPosition(const ezSimdVec4f& localPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float val);                    // [ property ]

private:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  ezSimdFloat m_fOneDivRadius;
};