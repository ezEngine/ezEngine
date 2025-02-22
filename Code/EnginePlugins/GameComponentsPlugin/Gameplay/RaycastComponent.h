
#pragma once

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class ezPhysicsWorldModuleInterface;

class ezRaycastComponentManager : public ezComponentManager<class ezRaycastComponent, ezBlockStorageType::Compact>
{
  using SUPER = ezComponentManager<class ezRaycastComponent, ezBlockStorageType::Compact>;

public:
  ezRaycastComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);
};

/// \brief A component which does a ray cast and positions a target object there.
///
/// This component does a ray cast along the forward axis of the game object it is attached to.
/// If this produces a hit the target object is placed there.
/// If no hit is found the target object is either placed at the maximum distance or deactivated depending on the component configuration.
///
/// This component can also trigger messages when objects enter the ray. E.g. when a player trips a laser detection beam.
/// To enable this set the trigger collision layer to another layer than the main ray cast and set a trigger message.
///
/// Sample setup:
///   m_uiCollisionLayerEndPoint = Default
///   m_uiCollisionLayerTrigger = Player
///   m_sTriggerMessage = "APlayerEnteredTheBeam"
///
/// This will lead to trigger messages being sent when a physics actor on the 'Player' layer comes between
/// the original hit on the default layer and the ray cast origin.
///
class EZ_GAMECOMPONENTS_DLL ezRaycastComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRaycastComponent, ezComponent, ezRaycastComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  void Deinitialize() override;
  void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRaycastComponent

public:
  ezRaycastComponent();
  ~ezRaycastComponent();

  float GetCurrentDistance() const { return m_fCurrentDistance; }     // [ scriptable ]
  ezVec3 GetCurrentEndPosition() const;                               // [ scriptable ]
  bool HasHit() const { return m_fCurrentDistance < m_fMaxDistance; } // [ scriptable ]

  void SetRaycastEndObject(const char* szReference);                  // [ property ]

  ezGameObjectHandle m_hRaycastEndObject;                             // [ property ]
  float m_fMaxDistance = 100.0f;                                      // [ property ]
  bool m_bForceTargetParentless = false;                              // [ property ]
  bool m_bDisableTargetObjectOnNoHit = false;                         // [ property ]
  ezUInt8 m_uiCollisionLayerEndPoint = 0;                             // [ property ]
  ezBitflags<ezPhysicsShapeType> m_ShapeTypesToHit;                   // [ property ]
  ezHashedString m_sChangeNotificationMsg;                            // [ property ]

private:
  void Update();

  const char* DummyGetter() const { return nullptr; }

  float m_fCurrentDistance = 0.0f;
  ezPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
};
