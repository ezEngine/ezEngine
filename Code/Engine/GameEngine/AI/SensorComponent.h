#pragma once

#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

class ezPhysicsWorldModuleInterface;

struct EZ_GAMEENGINE_DLL ezMsgSensorVisibleObjectsChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSensorVisibleObjectsChanged, ezEventMessage);

  ezArrayPtr<ezGameObjectHandle> m_VisibleObjects;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezSensorComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezSensorComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorComponent

public:
  ezSensorComponent();
  ~ezSensorComponent();

  virtual void GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_Objects) const = 0;

  void SetSpatialCategory(const char* szCategory); // [ property ]
  const char* GetSpatialCategory() const;          // [ property ]

  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

  ezEnum<ezUpdateRate> m_UpdateRate; // [ property ]

  ezArrayPtr<ezGameObjectHandle> GetLastVisibleObjects() const { return m_LastVisibleObjects; }

protected:
  void UpdateSpatialCategory();

  ezHashedString m_sSpatialCategory;
  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;

  friend class ezSensorWorldModule;
  mutable ezDynamicArray<ezGameObjectHandle> m_LastVisibleObjects;
};

//////////////////////////////////////////////////////////////////////////

using ezSensorSphereComponentManager = ezComponentManager<class ezSensorSphereComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezSensorSphereComponent : public ezSensorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSensorSphereComponent, ezSensorComponent, ezSensorSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorComponent

  virtual void GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_Objects) const;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorSphereComponent

public:
  ezSensorSphereComponent();
  ~ezSensorSphereComponent();

  float m_fRadius = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

class ezSensorWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezSensorWorldModule, ezWorldModule);
public:
  ezSensorWorldModule(ezWorld* pWorld);

  virtual void Initialize() override;

  void AddComponentToSchedule(ezSensorComponent* pComponent, ezUpdateRate::Enum updateRate);
  void RemoveComponentToSchedule(ezSensorComponent* pComponent);

private:
  void UpdateSensors(const ezWorldModule::UpdateContext& context);

  ezIntervalScheduler<ezComponentHandle> m_Scheduler;
  ezPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;

  ezDynamicArray<ezGameObject*> m_ObjectsInSensorVolume;
  ezDynamicArray<ezGameObjectHandle> m_VisibleObjects;
};
