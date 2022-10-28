#include <GameEngine/GameEnginePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState, 1, ezRTTINoAllocator)
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

void ezStateMachineState::OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const
{
}

void ezStateMachineState::Update(ezStateMachineInstance& instance, void* pInstanceData) const
{
}

ezResult ezStateMachineState::Serialize(ezStreamWriter& stream) const
{
  stream << m_sName;
  return EZ_SUCCESS;
}

ezResult ezStateMachineState::Deserialize(ezStreamReader& stream)
{
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  stream >> m_sName;
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineTransition, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezStateMachineTransition::Serialize(ezStreamWriter& stream) const
{
  return EZ_SUCCESS;
}

ezResult ezStateMachineTransition::Deserialize(ezStreamReader& stream)
{
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineInstanceDataTypeAttribute, 1, ezRTTIDefaultAllocator<ezStateMachineInstanceDataTypeAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

  auto& stateContext = m_States.ExpandAndGetRef();
  if (auto pInstanceDataAttribute = pState->GetDynamicRTTI()->GetAttributeByType<ezStateMachineInstanceDataTypeAttribute>())
  {
    m_InstanceDataAttributes.PushBack(pInstanceDataAttribute);
    stateContext.m_uiInstanceDataOffset = ezMemoryUtils::AlignSize(m_uiTotalInstanceDataSize, pInstanceDataAttribute->m_uiTypeAlignment);

    m_uiTotalInstanceDataSize = stateContext.m_uiInstanceDataOffset + pInstanceDataAttribute->m_uiTypeSize;
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

  auto& transitionContext = pTransitions->ExpandAndGetRef();
  transitionContext.m_pTransition = std::move(pTransistion);
  transitionContext.m_uiToStateIndex = uiToStateIndex;
}

constexpr ezTypeVersion s_StateMachineDescriptionVersion = 1;

ezResult ezStateMachineDescription::Serialize(ezStreamWriter& originalStream) const
{
  originalStream.WriteVersion(s_StateMachineDescriptionVersion);

  ezStringDeduplicationWriteContext stringDeduplicationWriteContext(originalStream);
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

    auto SerializeTransitions = [&](const TransitionArray& transitions, ezUInt32 uiFromStateIndex) -> ezResult {
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

ezResult ezStateMachineDescription::Deserialize(ezStreamReader& stream)
{
  const auto uiVersion = stream.ReadVersion(s_StateMachineDescriptionVersion);

  ezStringDeduplicationReadContext stringDeduplicationReadContext(stream);
  ezTypeVersionReadContext typeVersionReadContext(stream);

  ezStringBuilder sTypeName;

  // states
  {
    ezUInt32 uiNumStates = 0;
    stream >> uiNumStates;

    for (ezUInt32 i = 0; i < uiNumStates; ++i)
    {
      stream >> sTypeName;
      if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
      {
        ezUniquePtr<ezStateMachineState> pState = pType->GetAllocator()->Allocate<ezStateMachineState>();
        EZ_SUCCEED_OR_RETURN(pState->Deserialize(stream));

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
    stream >> uiNumTransitions;

    for (ezUInt32 i = 0; i < uiNumTransitions; ++i)
    {
      ezUInt32 uiFromStateIndex = 0;
      ezUInt32 uiToStateIndex = 0;

      stream >> uiFromStateIndex;
      stream >> uiToStateIndex;

      stream >> sTypeName;
      if (ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
      {
        ezUniquePtr<ezStateMachineTransition> pTransition = pType->GetAllocator()->Allocate<ezStateMachineTransition>();
        EZ_SUCCEED_OR_RETURN(pTransition->Deserialize(stream));

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

ezStateMachineInstance::ezStateMachineInstance(ezReflectedClass& owner, const ezSharedPtr<const ezStateMachineDescription>& pDescription /*= nullptr*/)
  : m_Owner(owner)
  , m_pDescription(pDescription)
{
  if (pDescription != nullptr && pDescription->m_uiTotalInstanceDataSize > 0)
  {
    m_InstanceData.SetCountUninitialized(pDescription->m_uiTotalInstanceDataSize);
    m_InstanceData.ZeroFill();

    ezUInt32 uiOffset = 0;
    for (auto pAttribute : pDescription->m_InstanceDataAttributes)
    {
      uiOffset = ezMemoryUtils::AlignSize(uiOffset, pAttribute->m_uiTypeAlignment);

      if (pAttribute->m_ConstructorFunction != nullptr)
      {
        pAttribute->m_ConstructorFunction(GetInstanceData(uiOffset));
      }

      uiOffset += pAttribute->m_uiTypeSize;
    }
  }
}

ezStateMachineInstance::~ezStateMachineInstance()
{
  ExitCurrentState(nullptr);

  m_pCurrentState = nullptr;
  m_uiCurrentStateIndex = ezInvalidIndex;

  if (m_pDescription != nullptr && m_pDescription->m_uiTotalInstanceDataSize > 0)
  {
    ezUInt32 uiOffset = 0;
    for (auto pAttribute : m_pDescription->m_InstanceDataAttributes)
    {
      uiOffset = ezMemoryUtils::AlignSize(uiOffset, pAttribute->m_uiTypeAlignment);

      if (pAttribute->m_DestructorFunction != nullptr)
      {
        pAttribute->m_DestructorFunction(GetInstanceData(uiOffset));
      }

      uiOffset += pAttribute->m_uiTypeSize;
    }

    m_InstanceData.Clear();
  }
}

ezResult ezStateMachineInstance::SetState(ezStateMachineState* pState)
{
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

void ezStateMachineInstance::Update()
{
  ezUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != ezInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->Update(*this, pInstanceData);
  }
}

void ezStateMachineInstance::SetBlackboard(const ezSharedPtr<ezBlackboard>& blackboard)
{
  m_pBlackboard = blackboard;
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
