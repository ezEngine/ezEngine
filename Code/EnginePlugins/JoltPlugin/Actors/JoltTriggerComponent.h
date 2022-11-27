#pragma once

#include <Core/Messages/TriggerMessage.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltTriggerComponentManager : public ezComponentManager<class ezJoltTriggerComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltTriggerComponentManager(ezWorld* pWorld);
  ~ezJoltTriggerComponentManager();

private:
  friend class ezJoltWorldModule;
  friend class ezJoltTriggerComponent;

  void UpdateMovingTriggers();

  ezSet<ezJoltTriggerComponent*> m_MovingTriggers;
};

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltTriggerComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltTriggerComponent, ezJoltActorComponent, ezJoltTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

public:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltTriggerComponent
public:
  ezJoltTriggerComponent();
  ~ezJoltTriggerComponent();

  void SetTriggerMessage(const char* szSz) { m_sTriggerMessage.Assign(szSz); }  // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]

protected:
  friend class ezJoltWorldModule;
  friend class ezJoltContactListener;

  void PostTriggerMessage(const ezGameObjectHandle& hOtherObject, ezTriggerState::Enum triggerState) const;

  ezHashedString m_sTriggerMessage;
  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
