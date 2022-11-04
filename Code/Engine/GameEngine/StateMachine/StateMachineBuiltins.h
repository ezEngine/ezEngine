#pragma once

#include <Core/Utils/Blackboard.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

/// \brief A state machine state implementation that represents another state machine nested within this state. This can be used to build hierarchical state machines.
class EZ_GAMEENGINE_DLL ezStateMachineState_NestedStateMachine : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_NestedStateMachine, ezStateMachineState);

public:
  ezStateMachineState_NestedStateMachine(ezStringView sName = ezStringView());
  ~ezStateMachineState_NestedStateMachine();

  virtual void OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;
  virtual void OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const override;
  virtual void Update(ezStateMachineInstance& instance, void* pInstanceData, ezTime deltaTime) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  void SetResource(const ezStateMachineResourceHandle& hResource);
  const ezStateMachineResourceHandle& GetResource() const { return m_hResource; }

  void SetResourceFile(const char* szFile); // [ property ]
  const char* GetResourceFile() const;      // [ property ]

  /// \brief Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

private:
  ezStateMachineResourceHandle m_hResource;
  ezHashedString m_sInitialState;

  // Should the inner state machine keep its current state on exit and re-enter or should it exit as well and re-enter the initial state again.
  bool m_bKeepCurrentStateOnExit = false;

  struct InstanceData
  {
    ezUniquePtr<ezStateMachineInstance> m_pStateMachineInstance;
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine state implementation that combines multiple sub states into one.
///
/// Can be used to build states in a more modular way. All calls are simply redirected to all sub states,
/// e.g. when entered it calls OnEnter on all its sub states.
class EZ_GAMEENGINE_DLL ezStateMachineState_Compound : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_Compound, ezStateMachineState);

public:
  ezStateMachineState_Compound(ezStringView sName = ezStringView());
  ~ezStateMachineState_Compound();

  virtual void OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;
  virtual void OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const override;
  virtual void Update(ezStateMachineInstance& instance, void* pInstanceData, ezTime deltaTime) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  ezHybridArray<ezStateMachineState*, 2> m_SubStates;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that checks the instance's blackboard for the given conditions.
class EZ_GAMEENGINE_DLL ezStateMachineTransition_BlackboardConditions : public ezStateMachineTransition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition_BlackboardConditions, ezStateMachineTransition);

public:
  ezStateMachineTransition_BlackboardConditions();
  ~ezStateMachineTransition_BlackboardConditions();

  virtual bool IsConditionMet(ezStateMachineInstance& instance, void* pInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  ezHybridArray<ezBlackboardCondition, 2> m_Conditions;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that triggers after the given time
class EZ_GAMEENGINE_DLL ezStateMachineTransition_Timeout : public ezStateMachineTransition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition_Timeout, ezStateMachineTransition);

public:
  ezStateMachineTransition_Timeout();
  ~ezStateMachineTransition_Timeout();

  virtual bool IsConditionMet(ezStateMachineInstance& instance, void* pInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& stream) const override;
  virtual ezResult Deserialize(ezStreamReader& stream) override;

  ezTime m_Timeout;
};
