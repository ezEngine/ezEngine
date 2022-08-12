#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

struct EZ_GAMEENGINE_DLL ezMsgStateMachineStateChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgStateMachineStateChanged, ezEventMessage);

  ezHashedString m_sNewStateName;

private:
  const char* GetNewStateName() const { return m_sNewStateName; }
  void SetNewStateName(const char* szName) { m_sNewStateName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

class ezStateMachineState_SendMsg : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_SendMsg, ezStateMachineState);

public:
  ezStateMachineState_SendMsg(const char* szName = nullptr);
  ~ezStateMachineState_SendMsg();

  virtual void OnEnter(ezStateMachineInstance& instance, void* pStateInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  ezTime m_Delay;
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

  /// \brief Returns the StateMachineInstance owned by this component
  ezStateMachineInstance* GetStateMachineInstance() { return m_pStateMachineInstance.Borrow(); }
  const ezStateMachineInstance* GetStateMachineInstance() const { return m_pStateMachineInstance.Borrow(); }

  void SetResource(const ezStateMachineResourceHandle& hResource);
  const ezStateMachineResourceHandle& GetResource() const { return m_hResource; }

  void SetResourceFile(const char* szFile); // [ property ]
  const char* GetResourceFile() const;      // [ property ]

  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

  bool SetState(const char* szName); // [ scriptable ]

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
