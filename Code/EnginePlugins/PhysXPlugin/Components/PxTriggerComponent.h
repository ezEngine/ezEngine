#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/Messages/TriggerTriggeredMessage.h>
#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxTriggerComponentManager : public ezComponentManager<class ezPxTriggerComponent, ezBlockStorageType::FreeList>
{
public:
  ezPxTriggerComponentManager(ezWorld* pWorld);
  ~ezPxTriggerComponentManager();

private:
  friend class ezPhysXWorldModule;
  friend class ezPxTriggerComponent;

  void UpdateKinematicActors();

  ezDynamicArray<ezPxTriggerComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxTriggerComponent : public ezPxActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxTriggerComponent, ezPxActorComponent, ezPxTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

public:
  virtual void OnSimulationStarted() override;
  virtual void Deinitialize() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxTriggerComponent
public:
  ezPxTriggerComponent();
  ~ezPxTriggerComponent();

  physx::PxRigidDynamic* GetActor() const { return m_pActor; }

  void SetKinematic(bool b);                         // [ property ]
  bool GetKinematic() const { return m_bKinematic; } // [ property ]

  void SetTriggerMessage(const char* sz) { m_sTriggerMessage.Assign(sz); }      // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]


protected:
  bool m_bKinematic = false;

  friend class ezPxSimulationEventCallback;

  physx::PxRigidDynamic* m_pActor = nullptr;

  void PostTriggerMessage(const ezComponent* pOtherComponent, ezTriggerState::Enum triggerState) const;

  ezHashedString m_sTriggerMessage;
  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
  ezPxUserData m_UserData;
};
