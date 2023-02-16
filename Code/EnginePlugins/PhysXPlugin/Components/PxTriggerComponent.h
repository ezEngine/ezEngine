#pragma once

#include <Core/Messages/TriggerMessage.h>
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
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxTriggerComponent
public:
  ezPxTriggerComponent();
  ~ezPxTriggerComponent();

  physx::PxRigidDynamic* GetActor() const { return m_pActor; }

  void SetTriggerMessage(const char* sz) { m_sTriggerMessage.Assign(sz); }      // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]

protected:
  friend class ezPhysXWorldModule;

  physx::PxRigidDynamic* m_pActor = nullptr;
  void PostTriggerMessage(const ezComponent* pOtherComponent, ezTriggerState::Enum triggerState) const;

  ezHashedString m_sTriggerMessage;
  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]

  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
};
