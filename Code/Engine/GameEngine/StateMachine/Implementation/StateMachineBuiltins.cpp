#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachineBuiltins.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState_NestedStateMachine, 1, ezRTTIDefaultAllocator<ezStateMachineState_NestedStateMachine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Resource", GetResource, SetResource)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_StateMachine", ezDependencyFlags::Package)),
    EZ_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    EZ_MEMBER_PROPERTY("KeepCurrentStateOnExit", m_bKeepCurrentStateOnExit),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineState_NestedStateMachine::ezStateMachineState_NestedStateMachine(ezStringView sName)
  : ezStateMachineState(sName)
{
}

ezStateMachineState_NestedStateMachine::~ezStateMachineState_NestedStateMachine() = default;

void ezStateMachineState_NestedStateMachine::OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;

  if (pStateMachineInstance == nullptr)
  {
    if (m_hResource.IsValid() == false)
      return;

    ezResourceLock<ezStateMachineResource> pStateMachineResource(m_hResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pStateMachineResource.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      ezLog::Error("Failed to load state machine '{}'", GetResource().GetResourceID());
      return;
    }

    pStateMachineInstance = pStateMachineResource->CreateInstance(ref_instance.GetOwner());
    pStateMachineInstance->SetBlackboard(ref_instance.GetBlackboard());
  }

  if (pStateMachineInstance->GetCurrentState() == nullptr)
  {
    pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
  }
}

void ezStateMachineState_NestedStateMachine::OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const
{
  if (m_bKeepCurrentStateOnExit == false)
  {
    auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
    if (pStateMachineInstance != nullptr)
    {
      pStateMachineInstance->SetState(nullptr).IgnoreResult();
    }
  }
}

void ezStateMachineState_NestedStateMachine::Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
  if (pStateMachineInstance != nullptr)
  {
    pStateMachineInstance->Update(deltaTime);
  }
}

ezResult ezStateMachineState_NestedStateMachine::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_hResource;
  inout_stream << m_sInitialState;
  inout_stream << m_bKeepCurrentStateOnExit;
  return EZ_SUCCESS;
}

ezResult ezStateMachineState_NestedStateMachine::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_hResource;
  inout_stream >> m_sInitialState;
  inout_stream >> m_bKeepCurrentStateOnExit;
  return EZ_SUCCESS;
}

bool ezStateMachineState_NestedStateMachine::GetInstanceDataDesc(ezInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezStateMachineState_NestedStateMachine::SetResource(const ezStateMachineResourceHandle& hResource)
{
  m_hResource = hResource;
}

void ezStateMachineState_NestedStateMachine::SetInitialState(const char* szName)
{
  m_sInitialState.Assign(szName);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState_Compound, 1, ezRTTIDefaultAllocator<ezStateMachineState_Compound>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("SubStates", m_SubStates)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineState_Compound::ezStateMachineState_Compound(ezStringView sName)
  : ezStateMachineState(sName)
{
}

ezStateMachineState_Compound::~ezStateMachineState_Compound()
{
  for (auto pSubState : m_SubStates)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

void ezStateMachineState_Compound::OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  for (ezUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnEnter(ref_instance, pSubInstanceData, pFromState);
  }
}

void ezStateMachineState_Compound::OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (ezUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnExit(ref_instance, pSubInstanceData, pToState);
  }
}

void ezStateMachineState_Compound::Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (ezUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->Update(ref_instance, pSubInstanceData, deltaTime);
  }
}

ezResult ezStateMachineState_Compound::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  const ezUInt32 uiNumSubStates = m_SubStates.GetCount();
  inout_stream << uiNumSubStates;

  for (auto pSubState : m_SubStates)
  {
    auto pStateType = pSubState->GetDynamicRTTI();
    ezTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    EZ_SUCCEED_OR_RETURN(pSubState->Serialize(inout_stream));
  }

  return EZ_SUCCESS;
}

ezResult ezStateMachineState_Compound::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);

  ezUInt32 uiNumSubStates = 0;
  inout_stream >> uiNumSubStates;
  m_SubStates.Reserve(uiNumSubStates);

  ezStringBuilder sTypeName;
  for (ezUInt32 i = 0; i < uiNumSubStates; ++i)
  {
    inout_stream >> sTypeName;
    if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
    {
      ezUniquePtr<ezStateMachineState> pSubState = pType->GetAllocator()->Allocate<ezStateMachineState>();
      EZ_SUCCEED_OR_RETURN(pSubState->Deserialize(inout_stream));

      m_SubStates.PushBack(pSubState.Release());
    }
    else
    {
      ezLog::Error("Unknown state machine state type '{}'", sTypeName);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

bool ezStateMachineState_Compound::GetInstanceDataDesc(ezInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubStates.GetArrayPtr(), out_desc);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezStateMachineLogicOperator, 1)
  EZ_ENUM_CONSTANTS(ezStateMachineLogicOperator::And, ezStateMachineLogicOperator::Or)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineTransition_BlackboardConditions, 1, ezRTTIDefaultAllocator<ezStateMachineTransition_BlackboardConditions>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Operator", ezStateMachineLogicOperator, m_Operator),
    EZ_ARRAY_MEMBER_PROPERTY("Conditions", m_Conditions),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineTransition_BlackboardConditions::ezStateMachineTransition_BlackboardConditions() = default;
ezStateMachineTransition_BlackboardConditions::~ezStateMachineTransition_BlackboardConditions() = default;

bool ezStateMachineTransition_BlackboardConditions::IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const
{
  if (m_Conditions.IsEmpty())
    return true;

  auto pBlackboard = ref_instance.GetBlackboard();
  if (pBlackboard == nullptr)
    return false;

  const bool bCheckFor = (m_Operator == ezStateMachineLogicOperator::Or) ? true : false;
  for (auto& condition : m_Conditions)
  {
    if (condition.IsConditionMet(*pBlackboard) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

ezResult ezStateMachineTransition_BlackboardConditions::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;
  return inout_stream.WriteArray(m_Conditions);
}

ezResult ezStateMachineTransition_BlackboardConditions::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Operator;
  return inout_stream.ReadArray(m_Conditions);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineTransition_Timeout, 1, ezRTTIDefaultAllocator<ezStateMachineTransition_Timeout>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Timeout", m_Timeout),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineTransition_Timeout::ezStateMachineTransition_Timeout() = default;
ezStateMachineTransition_Timeout::~ezStateMachineTransition_Timeout() = default;

bool ezStateMachineTransition_Timeout::IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetTimeInCurrentState() >= m_Timeout;
}

ezResult ezStateMachineTransition_Timeout::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Timeout;
  return EZ_SUCCESS;
}

ezResult ezStateMachineTransition_Timeout::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Timeout;
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineTransition_Compound, 1, ezRTTIDefaultAllocator<ezStateMachineTransition_Compound>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Operator", ezStateMachineLogicOperator, m_Operator),
    EZ_ARRAY_MEMBER_PROPERTY("SubTransitions", m_SubTransitions)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineTransition_Compound::ezStateMachineTransition_Compound() = default;

ezStateMachineTransition_Compound::~ezStateMachineTransition_Compound()
{
  for (auto pSubState : m_SubTransitions)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

bool ezStateMachineTransition_Compound::IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  const bool bCheckFor = (m_Operator == ezStateMachineLogicOperator::Or) ? true : false;
  for (ezUInt32 i = 0; i < m_SubTransitions.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    if (m_SubTransitions[i]->IsConditionMet(ref_instance, pSubInstanceData) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

ezResult ezStateMachineTransition_Compound::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;

  const ezUInt32 uiNumSubTransitions = m_SubTransitions.GetCount();
  inout_stream << uiNumSubTransitions;

  for (auto pSubTransition : m_SubTransitions)
  {
    auto pStateType = pSubTransition->GetDynamicRTTI();
    ezTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    EZ_SUCCEED_OR_RETURN(pSubTransition->Serialize(inout_stream));
  }

  return EZ_SUCCESS;
}

ezResult ezStateMachineTransition_Compound::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_Operator;

  ezUInt32 uiNumSubTransitions = 0;
  inout_stream >> uiNumSubTransitions;
  m_SubTransitions.Reserve(uiNumSubTransitions);

  ezStringBuilder sTypeName;
  for (ezUInt32 i = 0; i < uiNumSubTransitions; ++i)
  {
    inout_stream >> sTypeName;
    if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
    {
      ezUniquePtr<ezStateMachineTransition> pSubTransition = pType->GetAllocator()->Allocate<ezStateMachineTransition>();
      EZ_SUCCEED_OR_RETURN(pSubTransition->Deserialize(inout_stream));

      m_SubTransitions.PushBack(pSubTransition.Release());
    }
    else
    {
      ezLog::Error("Unknown state machine state type '{}'", sTypeName);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

bool ezStateMachineTransition_Compound::GetInstanceDataDesc(ezInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubTransitions.GetArrayPtr(), out_desc);
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineTransition_TransitionEvent, 1, ezRTTIDefaultAllocator<ezStateMachineTransition_TransitionEvent>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("EventName", m_sEventName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineTransition_TransitionEvent::ezStateMachineTransition_TransitionEvent() = default;
ezStateMachineTransition_TransitionEvent::~ezStateMachineTransition_TransitionEvent() = default;

bool ezStateMachineTransition_TransitionEvent::IsConditionMet(ezStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetCurrentTransitionEvent() == m_sEventName;
}

ezResult ezStateMachineTransition_TransitionEvent::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sEventName;
  return EZ_SUCCESS;
}

ezResult ezStateMachineTransition_TransitionEvent::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sEventName;
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineBuiltins);
