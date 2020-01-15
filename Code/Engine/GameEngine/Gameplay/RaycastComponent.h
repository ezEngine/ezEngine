
#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <GameEngine/Messages/TriggerTriggeredMessage.h>

#include <Core/World/Component.h>
#include <Core/World/World.h>

class ezPhysicsWorldModuleInterface;

typedef ezComponentManagerSimple<class ezRaycastComponent, ezComponentUpdateType::WhenSimulating> ezRaycastComponentManager;

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
///   m_sTrggerMessage = "APlayerEnteredTheBeam"
///
/// This will lead to trigger messages being sent when a physics actor on the 'Player' layer comes between
/// the original hit on the default layer and the ray cast origin.
///
class EZ_GAMEENGINE_DLL ezRaycastComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRaycastComponent, ezComponent, ezRaycastComponentManager);

public:

  ezRaycastComponent();
  ~ezRaycastComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:

  virtual void OnSimulationStarted() override;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

public:

  void SetTriggerMessage(const char* sz);     // [ property ]
  const char* GetTriggerMessage() const; // [ property ]

  void SetRaycastEndObject(const char* szReference); // [ property ]

  ezGameObjectHandle m_hRaycastEndObject; // [ property ]
  float m_fMaxDistance = 100.0f; // [ property ]
  bool m_bDisableTargetObjectOnNoHit = false; // [ property ]
  ezUInt8 m_uiCollisionLayerEndPoint = 0;     // [ property ]
  ezUInt8 m_uiCollisionLayerTrigger = 0;     // [ property ]


private:

  void Update();

  ezHashedString m_sTriggerMessage; // [ property ]
  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]


private:

  void PostTriggerMessage(ezTriggerState::Enum state, ezGameObjectHandle hObject);

  const char* DummyGetter() const { return nullptr; }

  ezPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;
  ezGameObjectHandle m_hLastTriggerObjectInRay;

};
