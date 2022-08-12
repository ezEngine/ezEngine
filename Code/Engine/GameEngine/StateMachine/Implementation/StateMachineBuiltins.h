#pragma once

#include <Core/Utils/Blackboard.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

class EZ_GAMEENGINE_DLL ezStateMachineState_NestedStateMachine : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_NestedStateMachine, ezStateMachineState);

public:
  ezStateMachineState_NestedStateMachine(const char* szName = nullptr);
  ~ezStateMachineState_NestedStateMachine();

  virtual void OnEnter(ezStateMachineInstance& instance, void* pStateInstanceData) const override;
  virtual void OnExit(ezStateMachineInstance& instance, void* pStateInstanceData) const override;
  virtual void Update(ezStateMachineInstance& instance, void* pStateInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  void SetResource(const ezStateMachineResourceHandle& hResource);
  const ezStateMachineResourceHandle& GetResource() const { return m_hResource; }

  void SetResourceFile(const char* szFile); // [ property ]
  const char* GetResourceFile() const;      // [ property ]

  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

private:
  ezStateMachineResourceHandle m_hResource;
  ezHashedString m_sInitialState;
  bool m_bResetOnEnter = true;
  bool m_bResetOnExit = true;

  struct InstanceData
  {
    ezUniquePtr<ezStateMachineInstance> m_pStateMachineInstance;
  };
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezStateMachineTransition_BlackboardConditions : public ezStateMachineTransition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition_BlackboardConditions, ezStateMachineTransition);

public:
  ezStateMachineTransition_BlackboardConditions();
  ~ezStateMachineTransition_BlackboardConditions();

  virtual bool IsConditionMet(ezStateMachineInstance& instance, void* pTransitionInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  ezHybridArray<ezBlackboardCondition, 2> m_Conditions;
};
