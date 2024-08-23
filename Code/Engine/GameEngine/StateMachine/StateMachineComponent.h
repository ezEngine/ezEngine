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

/// \brief A state machine state that sends a ezMsgStateMachineStateChanged on state enter or exit to the owner of the
/// state machine instance. Currently only works for ezStateMachineComponent.
///
/// Optionally it can also log a message on state enter or exit.
class ezStateMachineState_SendMsg : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_SendMsg, ezStateMachineState);

public:
  ezStateMachineState_SendMsg(ezStringView sName = ezStringView());
  ~ezStateMachineState_SendMsg();

  virtual void OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;
  virtual void OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezTime m_MessageDelay;

  bool m_bSendMessageOnEnter = true;
  bool m_bSendMessageOnExit = false;
  bool m_bLogOnEnter = false;
  bool m_bLogOnExit = false;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine state that sets the enabled flag on a game object and disables all other objects in the same group.
///
/// This state allows to easily switch the representation of a game object.
/// For instance you may have two objects states: normal and burning
/// You can basically just build two objects, one in the normal state, and one with all the effects needed for the fire.
/// Then you group both objects under a shared parent (e.g. with name 'visuals'), give both of them a name ('normal', 'burning') and disable one of them.
///
/// When the state machine transitions from the normal state to the burning state, you can then use this type of state
/// to say that from the 'visuals' group you want to activate the 'burning' object and deactivate all other objects in the same group.
///
/// Because the state activates one object and deactivates all others, you can have many different visuals and switch between them.
/// You can also only activate an object and keep the rest in the group as they are (e.g. to enable more and more effects).
/// If you only give a group path, but no object name, you can also use it to just disable all objects in a group.
/// If multiple objects in the same group have the same name, they will all get activated simultaneously.
///
/// Make sure that essential other objects (like the physics representation or other scripts) are located on other objects, that don't get deactivated.
class ezStateMachineState_SwitchObject : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_SwitchObject, ezStateMachineState);

public:
  ezStateMachineState_SwitchObject(ezStringView sName = ezStringView());
  ~ezStateMachineState_SwitchObject();

  virtual void OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezString m_sGroupPath;
  ezString m_sObjectToEnable;
  bool m_bDeactivateOthers = true;
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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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

  void SetResource(const ezStateMachineResourceHandle& hResource);                // [ property ]
  const ezStateMachineResourceHandle& GetResource() const { return m_hResource; } // [ property ]

  /// \brief Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

  /// \brief Sets the current state with the given name.
  bool SetState(ezStringView sName); // [ scriptable ]

  /// \brief Returns the name of the currently active state.
  ezStringView GetCurrentState() const; // [ scriptable ]

  /// \brief Sends a named event that state transitions can react to.
  void FireTransitionEvent(ezStringView sEvent);

  void SetBlackboardName(const char* szName);                         // [ property ]
  const char* GetBlackboardName() const { return m_sBlackboardName; } // [ property ]

private:
  friend class ezStateMachineState_SendMsg;
  void SendStateChangedMsg(ezMsgStateMachineStateChanged& msg, ezTime delay);
  void InstantiateStateMachine();
  void Update();

  ezStateMachineResourceHandle m_hResource;
  ezHashedString m_sInitialState;
  ezHashedString m_sBlackboardName;

  ezUniquePtr<ezStateMachineInstance> m_pStateMachineInstance;

  ezEventMessageSender<ezMsgStateMachineStateChanged> m_StateChangedSender; // [ event ]
};
