#pragma once

#if 0

#  include <Core/Messages/TriggerMessage.h>
#  include <JoltPlugin/Actors/JoltActorComponent.h>
#  include <JoltPlugin/Utilities/JoltUserData.h>

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltTriggerComponentManager : public ezComponentManager<class ezJoltTriggerComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltTriggerComponentManager(ezWorld* pWorld);
  ~ezJoltTriggerComponentManager();

private:
  friend class ezJoltWorldModule;
  friend class ezJoltTriggerComponent;

  void UpdateKinematicActors();

  ezDynamicArray<ezJoltTriggerComponent*> m_KinematicActorComponents;
};

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltTriggerComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltTriggerComponent, ezJoltActorComponent, ezJoltTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

public:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltTriggerComponent
public:
  ezJoltTriggerComponent();
  ~ezJoltTriggerComponent();

  //physx::PxRigidDynamic* GetActor() const { return m_pActor; }

  void SetTriggerMessage(const char* sz) { m_sTriggerMessage.Assign(sz); }      // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]

protected:
  friend class ezJoltWorldModule;

  //physx::PxRigidDynamic* m_pActor = nullptr;
  void PostTriggerMessage(const ezComponent* pOtherComponent, ezTriggerState::Enum triggerState) const;

  ezHashedString m_sTriggerMessage;
  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};

#endif
