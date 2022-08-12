#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/Implementation/StateMachineBuiltins.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState_NestedStateMachine, 1, ezRTTIDefaultAllocator<ezStateMachineState_NestedStateMachine>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_StateMachine")),
    EZ_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    EZ_MEMBER_PROPERTY("ResetOnEnter", m_bResetOnEnter)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ResetOnExit", m_bResetOnExit)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    EZ_INSTANCE_DATA_TYPE_ATTRIBUTE(ezStateMachineState_NestedStateMachine::InstanceData),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineState_NestedStateMachine::ezStateMachineState_NestedStateMachine(const char* szName /*= nullptr*/)
  : ezStateMachineState(szName)
{
}

ezStateMachineState_NestedStateMachine::~ezStateMachineState_NestedStateMachine() = default;

void ezStateMachineState_NestedStateMachine::OnEnter(ezStateMachineInstance& instance, void* pStateInstanceData) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pStateInstanceData)->m_pStateMachineInstance;

  bool bSetInitialState = m_bResetOnEnter;

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
    bSetInitialState = true;
  }

  if (bSetInitialState)
  {
    pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
  }
}

void ezStateMachineState_NestedStateMachine::OnExit(ezStateMachineInstance& instance, void* pStateInstanceData) const
{
  if (m_bResetOnExit)
  {
    auto& pStateMachineInstance = static_cast<InstanceData*>(pStateInstanceData)->m_pStateMachineInstance;
    if (pStateMachineInstance != nullptr)
    {
      pStateMachineInstance->SetState(nullptr);
    }
  }
}

void ezStateMachineState_NestedStateMachine::Update(ezStateMachineInstance& instance, void* pStateInstanceData) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pStateInstanceData)->m_pStateMachineInstance;
  if (pStateMachineInstance != nullptr)
  {
    pStateMachineInstance->Update();
  }
}

ezResult ezStateMachineState_NestedStateMachine::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_hResource;
  stream << m_sInitialState;
  stream << m_bResetOnEnter;
  stream << m_bResetOnExit;
  return EZ_SUCCESS;
}

ezResult ezStateMachineState_NestedStateMachine::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  stream >> m_hResource;
  stream >> m_sInitialState;
  stream >> m_bResetOnEnter;
  stream >> m_bResetOnExit;
  return EZ_SUCCESS;
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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineTransition_BlackboardConditions, 1, ezRTTIDefaultAllocator<ezStateMachineTransition_BlackboardConditions>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Conditions", m_Conditions),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineTransition_BlackboardConditions::ezStateMachineTransition_BlackboardConditions() = default;
ezStateMachineTransition_BlackboardConditions::~ezStateMachineTransition_BlackboardConditions() = default;

bool ezStateMachineTransition_BlackboardConditions::IsConditionMet(ezStateMachineInstance& instance, void* pTransitionInstanceData) const
{
  if (m_Conditions.IsEmpty())
    return true;

  auto pBlackboard = instance.GetBlackboard();
  if (pBlackboard == nullptr)
    return false;

  for (auto& condition : m_Conditions)
  {
    if (condition.IsConditionMet(*pBlackboard) == false)
      return false;
  }

  return true;
}

ezResult ezStateMachineTransition_BlackboardConditions::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  return stream.WriteArray(m_Conditions);
}

ezResult ezStateMachineTransition_BlackboardConditions::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));

  return stream.ReadArray(m_Conditions);
}
