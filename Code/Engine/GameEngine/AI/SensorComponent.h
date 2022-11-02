#pragma once

#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

class ezPhysicsWorldModuleInterface;

struct EZ_GAMEENGINE_DLL ezMsgSensorDetectedObjectsChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSensorDetectedObjectsChanged, ezEventMessage);

  ezArrayPtr<ezGameObjectHandle> m_DetectedObjects;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for sensor components that can be used for AI perception like vision or hearing.
///
/// Derived component classes implemented different shapes like sphere cylinder or cone.
/// All sensors do a query with the specified spatial category in the world's spatial system first, therefore it is necessary to have objects
/// with matching spatial category for the sensors to detect them. This can be achieved with components like e.g. ezMarkerComponent.
/// Visibility tests via raycasts are done afterwards by default but can be disabled.
/// The components store an array of all their currently detected objects and send an ezMsgSensorDetectedObjectsChanged message if this array changes.
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
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorComponent

public:
  ezSensorComponent();
  ~ezSensorComponent();

  virtual void GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_Objects) const = 0;
  virtual void DebugDrawSensorShape() const = 0;

  void SetSpatialCategory(const char* szCategory); // [ property ]
  const char* GetSpatialCategory() const;          // [ property ]

  bool m_bTestVisibility = true;  // [ property ]
  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

  void SetUpdateRate(const ezEnum<ezUpdateRate>& updateRate); // [ property ]
  const ezEnum<ezUpdateRate>& GetUpdateRate() const;          // [ property ]

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void SetColor(ezColorGammaUB color); // [ property ]
  ezColorGammaUB GetColor() const;     // [ property ]

  /// \brief Returns the list of objects that this sensor has detected during its last update
  ezArrayPtr<ezGameObjectHandle> GetLastDetectedObjects() const { return m_LastDetectedObjects; }

protected:
  void UpdateSpatialCategory();
  void UpdateScheduling();
  void UpdateDebugInfo();

  ezEnum<ezUpdateRate> m_UpdateRate;
  bool m_bShowDebugInfo = false;
  ezColorGammaUB m_Color = ezColorScheme::GetColorFor3DScene(ezColorScheme::Orange);

  ezHashedString m_sSpatialCategory;
  ezSpatialData::Category m_SpatialCategory = ezInvalidSpatialDataCategory;

  friend class ezSensorWorldModule;
  mutable ezDynamicArray<ezGameObjectHandle> m_LastDetectedObjects;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  mutable ezDynamicArray<ezVec3> m_LastOccludedObjectPositions;
#endif
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

  //////////////////////////////////////////////////////////////////////////
  // ezSensorComponent

  virtual void GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_Objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorSphereComponent

public:
  ezSensorSphereComponent();
  ~ezSensorSphereComponent();

  float m_fRadius = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using ezSensorCylinderComponentManager = ezComponentManager<class ezSensorCylinderComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezSensorCylinderComponent : public ezSensorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSensorCylinderComponent, ezSensorComponent, ezSensorCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorComponent

  virtual void GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_Objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorCylinderComponent

public:
  ezSensorCylinderComponent();
  ~ezSensorCylinderComponent();

  float m_fRadius = 10.0f; // [ property ]
  float m_fHeight = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using ezSensorConeComponentManager = ezComponentManager<class ezSensorConeComponent, ezBlockStorageType::Compact>;

class EZ_GAMEENGINE_DLL ezSensorConeComponent : public ezSensorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSensorConeComponent, ezSensorComponent, ezSensorConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorComponent

  virtual void GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_Objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSensorConeComponent

public:
  ezSensorConeComponent();
  ~ezSensorConeComponent();

  float m_fNearDistance = 0.0f;             // [ property ]
  float m_fFarDistance = 10.0f;             // [ property ]
  ezAngle m_Angle = ezAngle::Degree(90.0f); // [ property ]
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

  void AddComponentForDebugRendering(ezSensorComponent* pComponent);
  void RemoveComponentForDebugRendering(ezSensorComponent* pComponent);

private:
  void UpdateSensors(const ezWorldModule::UpdateContext& context);
  void DebugDrawSensors(const ezWorldModule::UpdateContext& context);

  ezIntervalScheduler<ezComponentHandle> m_Scheduler;
  ezPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;

  ezDynamicArray<ezGameObject*> m_ObjectsInSensorVolume;
  ezDynamicArray<ezGameObjectHandle> m_DetectedObjects;

  ezDynamicArray<ezComponentHandle> m_DebugComponents;
};
