#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/Component.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetName),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_OnEnter, In, "StateMachineInstance", In, "FromState")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezStateMachineState_ScriptBaseClassFunctions::OnEnter)),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_OnExit, In, "StateMachineInstance", In, "ToState")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezStateMachineState_ScriptBaseClassFunctions::OnExit)),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_Update, In, "StateMachineInstance", In, "DeltaTime")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezStateMachineState_ScriptBaseClassFunctions::Update)),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState_Empty, 1, ezRTTIDefaultAllocator<ezStateMachineState_Empty>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezHiddenAttribute(),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineState::ezStateMachineState(ezStringView sName)
{
  m_sName.Assign(sName);
}

void ezStateMachineState::SetName(ezStringView sName)
{
  EZ_ASSERT_DEV(m_sName.IsEmpty(), "Name can't be changed afterwards");
  m_sName.Assign(sName);
}

void ezStateMachineState::OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const
{
}

void ezStateMachineState::Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const
{
}

ezResult ezStateMachineState::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  return EZ_SUCCESS;
}

ezResult ezStateMachineState::Deserialize(ezStreamReader& inout_stream)
{
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sName;
  return EZ_SUCCESS;
}

bool ezStateMachineState::GetInstanceDataDesc(ezInstanceDataDesc& out_desc)
{
  return false;
}

void ezStateMachineState::Reflection_OnEnter(ezStateMachineInstance* pStateMachineInstance, const ezStateMachineState* pFromState)
{
}

void ezStateMachineState::Reflection_OnExit(ezStateMachineInstance* pStateMachineInstance, const ezStateMachineState* pToState)
{
}

void ezStateMachineState::Reflection_Update(ezStateMachineInstance* pStateMachineInstance, ezTime deltaTime)
{
}

ezStateMachineState_Empty::ezStateMachineState_Empty(ezStringView sName)
  : ezStateMachineState(sName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineTransition, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezStateMachineTransition::Serialize(ezStreamWriter& inout_stream) const
{
  return EZ_SUCCESS;
}

ezResult ezStateMachineTransition::Deserialize(ezStreamReader& inout_stream)
{
  return EZ_SUCCESS;
}

bool ezStateMachineTransition::GetInstanceDataDesc(ezInstanceDataDesc& out_desc)
{
  return false;
}

//////////////////////////////////////////////////////////////////////////

ezStateMachineDescription::ezStateMachineDescription() = default;
ezStateMachineDescription::~ezStateMachineDescription() = default;

ezUInt32 ezStateMachineDescription::AddState(ezUniquePtr<ezStateMachineState>&& pState)
{
  const ezUInt32 uiIndex = m_States.GetCount();

  auto& sStateName = pState->GetNameHashed();
  if (sStateName.IsEmpty() == false)
  {
    EZ_VERIFY(m_StateNameToIndexTable.Contains(sStateName) == false, "A state with name '{}' already exists.", sStateName);
    m_StateNameToIndexTable.Insert(sStateName, uiIndex);
  }

  StateContext& stateContext = m_States.ExpandAndGetRef();

  ezInstanceDataDesc instanceDataDesc;
  if (pState->GetInstanceDataDesc(instanceDataDesc))
  {
    stateContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  stateContext.m_pState = std::move(pState);

  return uiIndex;
}

void ezStateMachineDescription::AddTransition(ezUInt32 uiFromStateIndex, ezUInt32 uiToStateIndex, ezUniquePtr<ezStateMachineTransition>&& pTransistion)
{
  EZ_ASSERT_DEV(uiFromStateIndex != uiToStateIndex, "Can't add a transition to itself");

  TransitionArray* pTransitions = nullptr;
  if (uiFromStateIndex == ezInvalidIndex)
  {
    pTransitions = &m_FromAnyTransitions;
  }
  else
  {
    EZ_ASSERT_DEV(uiFromStateIndex < m_States.GetCount(), "Invalid from state index {}", uiFromStateIndex);
    pTransitions = &m_States[uiFromStateIndex].m_Transitions;
  }

  EZ_ASSERT_DEV(uiToStateIndex < m_States.GetCount(), "Invalid to state index {}", uiToStateIndex);

  TransitionContext& transitionContext = pTransitions->ExpandAndGetRef();

  ezInstanceDataDesc instanceDataDesc;
  if (pTransistion->GetInstanceDataDesc(instanceDataDesc))
  {
    transitionContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  transitionContext.m_pTransition = std::move(pTransistion);
  transitionContext.m_uiToStateIndex = uiToStateIndex;
}

constexpr ezTypeVersion s_StateMachineDescriptionVersion = 1;

ezResult ezStateMachineDescription::Serialize(ezStreamWriter& ref_originalStream) const
{
  ref_originalStream.WriteVersion(s_StateMachineDescriptionVersion);

  ezStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_originalStream);
  ezTypeVersionWriteContext typeVersionWriteContext;
  auto& stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  ezUInt32 uiNumTransitions = m_FromAnyTransitions.GetCount();

  // states
  {
    const ezUInt32 uiNumStates = m_States.GetCount();
    stream << uiNumStates;

    for (auto& stateContext : m_States)
    {
      auto pStateType = stateContext.m_pState->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pStateType);

      stream << pStateType->GetTypeName();
      EZ_SUCCEED_OR_RETURN(stateContext.m_pState->Serialize(stream));

      uiNumTransitions += stateContext.m_Transitions.GetCount();
    }
  }

  // transitions
  {
    stream << uiNumTransitions;

    auto SerializeTransitions = [&](const TransitionArray& transitions, ezUInt32 uiFromStateIndex) -> ezResult
    {
      for (auto& transitionContext : transitions)
      {
        const ezUInt32 uiToStateIndex = transitionContext.m_uiToStateIndex;

        stream << uiFromStateIndex;
        stream << uiToStateIndex;

        auto pTransitionType = transitionContext.m_pTransition->GetDynamicRTTI();
        typeVersionWriteContext.AddType(pTransitionType);

        stream << pTransitionType->GetTypeName();
        EZ_SUCCEED_OR_RETURN(transitionContext.m_pTransition->Serialize(stream));
      }

      return EZ_SUCCESS;
    };

    EZ_SUCCEED_OR_RETURN(SerializeTransitions(m_FromAnyTransitions, ezInvalidIndex));

    for (ezUInt32 uiFromStateIndex = 0; uiFromStateIndex < m_States.GetCount(); ++uiFromStateIndex)
    {
      auto& transitions = m_States[uiFromStateIndex].m_Transitions;

      EZ_SUCCEED_OR_RETURN(SerializeTransitions(transitions, uiFromStateIndex));
    }
  }

  EZ_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  EZ_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return EZ_SUCCESS;
}

ezResult ezStateMachineDescription::Deserialize(ezStreamReader& inout_stream)
{
  const auto uiVersion = inout_stream.ReadVersion(s_StateMachineDescriptionVersion);
  EZ_IGNORE_UNUSED(uiVersion);

  ezStringDeduplicationReadContext stringDeduplicationReadContext(inout_stream);
  ezTypeVersionReadContext typeVersionReadContext(inout_stream);

  ezStringBuilder sTypeName;

  // states
  {
    ezUInt32 uiNumStates = 0;
    inout_stream >> uiNumStates;

    for (ezUInt32 i = 0; i < uiNumStates; ++i)
    {
      inout_stream >> sTypeName;
      if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
      {
        ezUniquePtr<ezStateMachineState> pState = pType->GetAllocator()->Allocate<ezStateMachineState>();
        EZ_SUCCEED_OR_RETURN(pState->Deserialize(inout_stream));

        EZ_VERIFY(AddState(std::move(pState)) == i, "Implementation error");
      }
      else
      {
        ezLog::Error("Unknown state machine state type '{}'", sTypeName);
        return EZ_FAILURE;
      }
    }
  }

  // transitions
  {
    ezUInt32 uiNumTransitions = 0;
    inout_stream >> uiNumTransitions;

    for (ezUInt32 i = 0; i < uiNumTransitions; ++i)
    {
      ezUInt32 uiFromStateIndex = 0;
      ezUInt32 uiToStateIndex = 0;

      inout_stream >> uiFromStateIndex;
      inout_stream >> uiToStateIndex;

      inout_stream >> sTypeName;
      if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
      {
        ezUniquePtr<ezStateMachineTransition> pTransition = pType->GetAllocator()->Allocate<ezStateMachineTransition>();
        EZ_SUCCEED_OR_RETURN(pTransition->Deserialize(inout_stream));

        AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
      }
      else
      {
        ezLog::Error("Unknown state machine transition type '{}'", sTypeName);
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezStateMachineInstance, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_SetState, In, "StateName"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentState),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetTimeInCurrentState),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOwnerComponent),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_GetBlackboard),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineInstance::ezStateMachineInstance(ezReflectedClass& ref_owner, const ezSharedPtr<const ezStateMachineDescription>& pDescription /*= nullptr*/)
  : m_Owner(ref_owner)
  , m_pDescription(pDescription)
{
  if (pDescription != nullptr)
  {
    m_InstanceData = pDescription->m_InstanceDataAllocator.AllocateAndConstruct();
  }
}

ezStateMachineInstance::~ezStateMachineInstance()
{
  ExitCurrentState(nullptr);

  m_pCurrentState = nullptr;
  m_uiCurrentStateIndex = ezInvalidIndex;

  if (m_pDescription != nullptr)
  {
    m_pDescription->m_InstanceDataAllocator.DestructAndDeallocate(m_InstanceData);
  }
}

ezResult ezStateMachineInstance::SetState(ezStateMachineState* pState)
{
  if (m_pCurrentState == pState)
    return EZ_SUCCESS;

  if (pState != nullptr && m_pDescription != nullptr)
  {
    return SetState(pState->GetNameHashed());
  }

  const auto pFromState = m_pCurrentState;
  const auto pToState = pState;

  ExitCurrentState(pToState);

  m_pCurrentState = pState;
  m_uiCurrentStateIndex = ezInvalidIndex;
  m_pCurrentTransitions = nullptr;

  EnterCurrentState(pFromState);

  return EZ_SUCCESS;
}

ezResult ezStateMachineInstance::SetState(const ezHashedString& sStateName)
{
  EZ_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by name");

  ezUInt32 uiStateIndex = 0;
  if (m_pDescription->m_StateNameToIndexTable.TryGetValue(sStateName, uiStateIndex))
  {
    SetStateInternal(uiStateIndex);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezStateMachineInstance::SetState(ezUInt32 uiStateIndex)
{
  EZ_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by index");

  if (uiStateIndex < m_pDescription->m_States.GetCount())
  {
    SetStateInternal(uiStateIndex);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezStateMachineInstance::SetStateOrFallback(const ezHashedString& sStateName, ezUInt32 uiFallbackStateIndex /*= 0*/)
{
  if (SetState(sStateName).Failed())
  {
    return SetState(uiFallbackStateIndex);
  }

  return EZ_SUCCESS;
}

void ezStateMachineInstance::Update(ezTime deltaTime)
{
  ezUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != ezInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->Update(*this, pInstanceData, deltaTime);
  }

  m_TimeInCurrentState += deltaTime;
}

ezWorld* ezStateMachineInstance::GetOwnerWorld()
{
  if (auto pComponent = ezDynamicCast<ezComponent*>(&m_Owner))
  {
    return pComponent->GetWorld();
  }

  return nullptr;
}

void ezStateMachineInstance::SetBlackboard(const ezSharedPtr<ezBlackboard>& pBlackboard)
{
  m_pBlackboard = pBlackboard;
}

void ezStateMachineInstance::FireTransitionEvent(ezStringView sEvent)
{
  m_sCurrentTransitionEvent = sEvent;

  ezUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != ezInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  m_sCurrentTransitionEvent = {};
}

bool ezStateMachineInstance::Reflection_SetState(const ezHashedString& sStateName)
{
  return SetState(sStateName).Succeeded();
}

ezComponent* ezStateMachineInstance::Reflection_GetOwnerComponent() const
{
  return ezDynamicCast<ezComponent*>(&m_Owner);
}

void ezStateMachineInstance::SetStateInternal(ezUInt32 uiStateIndex)
{
  if (m_uiCurrentStateIndex == uiStateIndex)
    return;

  const auto& stateContext = m_pDescription->m_States[uiStateIndex];
  const auto pFromState = m_pCurrentState;
  const auto pToState = stateContext.m_pState.Borrow();

  ExitCurrentState(pToState);

  m_pCurrentState = pToState;
  m_uiCurrentStateIndex = uiStateIndex;
  m_pCurrentTransitions = &stateContext.m_Transitions;

  EnterCurrentState(pFromState);
}

void ezStateMachineInstance::EnterCurrentState(const ezStateMachineState* pFromState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnEnter(*this, pInstanceData, pFromState);

    m_TimeInCurrentState = ezTime::MakeZero();
  }
}

void ezStateMachineInstance::ExitCurrentState(const ezStateMachineState* pToState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnExit(*this, pInstanceData, pToState);
  }
}

ezUInt32 ezStateMachineInstance::FindNewStateToTransitionTo()
{
  if (m_pCurrentTransitions != nullptr)
  {
    for (auto& transitionContext : *m_pCurrentTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  if (m_pDescription != nullptr)
  {
    for (auto& transitionContext : m_pDescription->m_FromAnyTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  return ezInvalidIndex;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachine);
