#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachineBuiltins.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState_NestedStateMachine, 1, ezRTTIDefaultAllocator<ezStateMachineState_NestedStateMachine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_StateMachine")),
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

void ezStateMachineState_NestedStateMachine::OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;

  if (pStateMachineInstance == nullptr)
  {
    if (m_hResource.IsValid() == false)
      return;

    ezResourceLock<ezStateMachineResource> pStateMachineResource(m_hResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pStateMachineResource.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      ezLog::Error("Failed to load state machine '{}'", GetResourceFile());
      return;
    }

    pStateMachineInstance = pStateMachineResource->CreateInstance(instance.GetOwner());
    pStateMachineInstance->SetBlackboard(instance.GetBlackboard());
  }

  if (pStateMachineInstance->GetCurrentState() == nullptr)
  {
    pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
  }
}

void ezStateMachineState_NestedStateMachine::OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const
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

void ezStateMachineState_NestedStateMachine::Update(ezStateMachineInstance& instance, void* pInstanceData, ezTime deltaTime) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
  if (pStateMachineInstance != nullptr)
  {
    pStateMachineInstance->Update(deltaTime);
  }
}

ezResult ezStateMachineState_NestedStateMachine::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_hResource;
  stream << m_sInitialState;
  stream << m_bKeepCurrentStateOnExit;
  return EZ_SUCCESS;
}

ezResult ezStateMachineState_NestedStateMachine::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  stream >> m_hResource;
  stream >> m_sInitialState;
  stream >> m_bKeepCurrentStateOnExit;
  return EZ_SUCCESS;
}

bool ezStateMachineState_NestedStateMachine::GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void ezStateMachineState_NestedStateMachine::SetResource(const ezStateMachineResourceHandle& hResource)
{
  m_hResource = hResource;
}

void ezStateMachineState_NestedStateMachine::SetResourceFile(const char* szFile)
{
  ezStateMachineResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezStateMachineResource>(szFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* ezStateMachineState_NestedStateMachine::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
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

void ezStateMachineState_Compound::OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  for (ezUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnEnter(instance, pSubInstanceData, pFromState);
  }
}

void ezStateMachineState_Compound::OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (ezUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnExit(instance, pSubInstanceData, pToState);
  }
}

void ezStateMachineState_Compound::Update(ezStateMachineInstance& instance, void* pInstanceData, ezTime deltaTime) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (ezUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->Update(instance, pSubInstanceData, deltaTime);
  }
}

ezResult ezStateMachineState_Compound::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  const ezUInt32 uiNumSubStates = m_SubStates.GetCount();
  stream << uiNumSubStates;

  for (auto pSubState : m_SubStates)
  {
    auto pStateType = pSubState->GetDynamicRTTI();
    ezTypeVersionWriteContext::GetContext()->AddType(pStateType);

    stream << pStateType->GetTypeName();
    EZ_SUCCEED_OR_RETURN(pSubState->Serialize(stream));
  }

  return EZ_SUCCESS;
}

ezResult ezStateMachineState_Compound::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  ezUInt32 uiNumSubStates = 0;
  stream >> uiNumSubStates;
  m_SubStates.Reserve(uiNumSubStates);

  ezStringBuilder sTypeName;
  for (ezUInt32 i = 0; i < uiNumSubStates; ++i)
  {
    stream >> sTypeName;
    if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
    {
      ezUniquePtr<ezStateMachineState> pSubState = pType->GetAllocator()->Allocate<ezStateMachineState>();
      EZ_SUCCEED_OR_RETURN(pSubState->Deserialize(stream));

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

bool ezStateMachineState_Compound::GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc)
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

bool ezStateMachineTransition_BlackboardConditions::IsConditionMet(ezStateMachineInstance& instance, void* pInstanceData) const
{
  if (m_Conditions.IsEmpty())
    return true;

  auto pBlackboard = instance.GetBlackboard();
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

ezResult ezStateMachineTransition_BlackboardConditions::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_Operator;
  return stream.WriteArray(m_Conditions);
}

ezResult ezStateMachineTransition_BlackboardConditions::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));

  stream >> m_Operator;
  return stream.ReadArray(m_Conditions);
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

bool ezStateMachineTransition_Timeout::IsConditionMet(ezStateMachineInstance& instance, void* pInstanceData) const
{
  return instance.GetTimeInCurrentState() >= m_Timeout;
}

ezResult ezStateMachineTransition_Timeout::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_Timeout;
  return EZ_SUCCESS;
}

ezResult ezStateMachineTransition_Timeout::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));

  stream >> m_Timeout;
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

bool ezStateMachineTransition_Compound::IsConditionMet(ezStateMachineInstance& instance, void* pInstanceData) const
{
  auto pData = static_cast<ezStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  const bool bCheckFor = (m_Operator == ezStateMachineLogicOperator::Or) ? true : false;
  for (ezUInt32 i = 0; i < m_SubTransitions.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    if (m_SubTransitions[i]->IsConditionMet(instance, pSubInstanceData) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

ezResult ezStateMachineTransition_Compound::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_Operator;

  const ezUInt32 uiNumSubTransitions = m_SubTransitions.GetCount();
  stream << uiNumSubTransitions;

  for (auto pSubTransition : m_SubTransitions)
  {
    auto pStateType = pSubTransition->GetDynamicRTTI();
    ezTypeVersionWriteContext::GetContext()->AddType(pStateType);

    stream << pStateType->GetTypeName();
    EZ_SUCCEED_OR_RETURN(pSubTransition->Serialize(stream));
  }

  return EZ_SUCCESS;
}

ezResult ezStateMachineTransition_Compound::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  stream >> m_Operator;

  ezUInt32 uiNumSubTransitions = 0;
  stream >> uiNumSubTransitions;
  m_SubTransitions.Reserve(uiNumSubTransitions);

  ezStringBuilder sTypeName;
  for (ezUInt32 i = 0; i < uiNumSubTransitions; ++i)
  {
    stream >> sTypeName;
    if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
    {
      ezUniquePtr<ezStateMachineTransition> pSubTransition = pType->GetAllocator()->Allocate<ezStateMachineTransition>();
      EZ_SUCCEED_OR_RETURN(pSubTransition->Deserialize(stream));

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

bool ezStateMachineTransition_Compound::GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubTransitions.GetArrayPtr(), out_desc);
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineBuiltins);

