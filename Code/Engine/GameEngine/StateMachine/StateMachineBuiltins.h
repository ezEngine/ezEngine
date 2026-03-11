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

  virtual void OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;
  virtual void OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const override;
  virtual void Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) override;

  void SetResource(const ezStateMachineResourceHandle& hResource);                // [ property ]
  const ezStateMachineResourceHandle& GetResource() const { return m_hResource; } // [ property ]

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

  virtual void OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;
  virtual void OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const override;
  virtual void Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) override;

  ezSmallArray<ezStateMachineState*, 2> m_SubStates;

private:
  ezStateMachineInternal::Compound m_Compound;
};

//////////////////////////////////////////////////////////////////////////

/// \brief An enum that represents the operator of a comparison
struct EZ_GAMEENGINE_DLL ezStateMachineLogicOperator
{
  using StorageType = ezUInt8;

  enum Enum
  {
    And,
    Or,

    Default = And
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezStateMachineLogicOperator);

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that checks the instance's blackboard for the given conditions.
class EZ_GAMEENGINE_DLL ezStateMachineTransition_BlackboardConditions : public ezStateMachineTransition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition_BlackboardConditions, ezStateMachineTransition);

public:
  ezStateMachineTransition_BlackboardConditions();
  ~ezStateMachineTransition_BlackboardConditions();

  virtual bool IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezEnum<ezStateMachineLogicOperator> m_Operator;
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

  virtual bool IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezTime m_Timeout;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that combines multiple sub transition into one.
///
/// Can be used to build transitions in a more modular way. All calls are simply redirected to all sub transitions
/// and then combined with the given logic operator (AND, OR).
class EZ_GAMEENGINE_DLL ezStateMachineTransition_Compound : public ezStateMachineTransition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition_Compound, ezStateMachineTransition);

public:
  ezStateMachineTransition_Compound();
  ~ezStateMachineTransition_Compound();

  virtual bool IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) override;

  ezEnum<ezStateMachineLogicOperator> m_Operator;
  ezSmallArray<ezStateMachineTransition*, 2> m_SubTransitions;

private:
  ezStateMachineInternal::Compound m_Compound;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that triggers when a 'transition event' is sent.
class EZ_GAMEENGINE_DLL ezStateMachineTransition_TransitionEvent : public ezStateMachineTransition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineTransition_TransitionEvent, ezStateMachineTransition);

public:
  ezStateMachineTransition_TransitionEvent();
  ~ezStateMachineTransition_TransitionEvent();

  virtual bool IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezHashedString m_sEventName;
};
