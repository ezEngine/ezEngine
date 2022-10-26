#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

/// \brief Message that is sent by ezStateMachineState_SendMsg once the state is entered.
struct EZ_GAMEENGINE_DLL ezMsgStateMachineStateChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgStateMachineStateChanged, ezEventMessage);

  ezHashedString m_sOldStateName;
  ezHashedString m_sNewStateName;

private:
  const char* GetOldStateName() const { return m_sOldStateName; }
  void SetOldStateName(const char* szName) { m_sOldStateName.Assign(szName); }

  const char* GetNewStateName() const { return m_sNewStateName; }
  void SetNewStateName(const char* szName) { m_sNewStateName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine state implementation that sends a ezMsgStateMachineStateChanged on state enter or exit to the owner of the
/// state machine instance. Currently only works for ezStateMachineComponent.
///
/// Optionally it can also log a message on state enter or exit.
class ezStateMachineState_SendMsg : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_SendMsg, ezStateMachineState);

public:
  ezStateMachineState_SendMsg(ezStringView sName = ezStringView());
  ~ezStateMachineState_SendMsg();

  virtual void OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;
  virtual void OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  ezTime m_MessageDelay;

  bool m_bSendMessageOnEnter = true;
  bool m_bSendMessageOnExit = false;
  bool m_bLogOnEnter = false;
  bool m_bLogOnExit = false;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezStateMachineComponentManager : public ezComponentManager<class ezStateMachineComponent, ezBlockStorageType::Compact>
{
public:
  ezStateMachineComponentManager(ezWorld* pWorld);
  ~ezStateMachineComponentManager();

  virtual void Initialize() override;

  void Update(const ezWorldModule::UpdateContext& context);

private:
  void ResourceEventHandler(const ezResourceEvent& e);

  ezHashSet<ezComponentHandle> m_ComponentsToReload;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A component that holds an ezStateMachineInstance using the ezStateMachineDescription from the resource assigned to this component.
class EZ_GAMEENGINE_DLL ezStateMachineComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezStateMachineComponent, ezComponent, ezStateMachineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezStateMachineComponent

public:
  ezStateMachineComponent();
  ezStateMachineComponent(ezStateMachineComponent&& other);
  ~ezStateMachineComponent();

  ezStateMachineComponent& operator=(ezStateMachineComponent&& other);

  /// \brief Returns the ezStateMachineInstance owned by this component
  ezStateMachineInstance* GetStateMachineInstance() { return m_pStateMachineInstance.Borrow(); }
  const ezStateMachineInstance* GetStateMachineInstance() const { return m_pStateMachineInstance.Borrow(); }

  void SetResource(const ezStateMachineResourceHandle& hResource);
  const ezStateMachineResourceHandle& GetResource() const { return m_hResource; }

  void SetResourceFile(const char* szFile); // [ property ]
  const char* GetResourceFile() const;      // [ property ]

  /// \brief Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void SetInitialState(const char* szName);                        // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

  /// \brief Sets the current state with the given name.
  bool SetState(ezStringView sName); // [ scriptable ]

private:
  friend class ezStateMachineState_SendMsg;
  void SendStateChangedMsg(ezMsgStateMachineStateChanged& msg, ezTime delay);
  void InstantiateStateMachine();
  void Update();

  ezStateMachineResourceHandle m_hResource;
  ezHashedString m_sInitialState;

  ezUniquePtr<ezStateMachineInstance> m_pStateMachineInstance;

  ezEventMessageSender<ezMsgStateMachineStateChanged> m_StateChangedSender; // [ event ]
};
