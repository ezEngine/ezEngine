#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/StateMachine/StateMachineComponent.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgStateMachineStateChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgStateMachineStateChanged, 1, ezRTTIDefaultAllocator<ezMsgStateMachineStateChanged>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("OldStateName", GetOldStateName, SetOldStateName),
    EZ_ACCESSOR_PROPERTY("NewStateName", GetNewStateName, SetNewStateName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
      new ezAutoGenVisScriptMsgHandler()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineState_SendMsg, 1, ezRTTIDefaultAllocator<ezStateMachineState_SendMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MessageDelay", m_MessageDelay),
    EZ_MEMBER_PROPERTY("SendMessageOnEnter", m_bSendMessageOnEnter)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("SendMessageOnExit", m_bSendMessageOnExit),
    EZ_MEMBER_PROPERTY("LogOnEnter", m_bLogOnEnter),
    EZ_MEMBER_PROPERTY("LogOnExit", m_bLogOnExit),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStateMachineState_SendMsg::ezStateMachineState_SendMsg(ezStringView sName)
  : ezStateMachineState(sName)
{
}

ezStateMachineState_SendMsg::~ezStateMachineState_SendMsg() = default;

void ezStateMachineState_SendMsg::OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const
{
  ezHashedString sFromState = (pFromState != nullptr) ? pFromState->GetNameHashed() : ezHashedString();

  if (m_bSendMessageOnEnter)
  {
    if (auto pOwner = ezDynamicCast<ezStateMachineComponent*>(&instance.GetOwner()))
    {
      ezMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = sFromState;
      msg.m_sNewStateName = GetNameHashed();

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnEnter)
  {
    ezLog::Info("State Machine: Entering '{}' State from '{}'", GetNameHashed(), sFromState);
  }
}

void ezStateMachineState_SendMsg::OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const
{
  ezHashedString sToState = (pToState != nullptr) ? pToState->GetNameHashed() : ezHashedString();

  if (m_bSendMessageOnExit)
  {
    if (auto pOwner = ezDynamicCast<ezStateMachineComponent*>(&instance.GetOwner()))
    {
      ezMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = GetNameHashed();
      msg.m_sNewStateName = sToState;

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnExit)
  {
    ezLog::Info("State Machine: Exiting '{}' State to '{}'", GetNameHashed(), sToState);
  }
}

ezResult ezStateMachineState_SendMsg::Serialize(ezStreamWriter& stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_MessageDelay;
  stream << m_bSendMessageOnEnter;
  stream << m_bSendMessageOnExit;
  stream << m_bLogOnEnter;
  stream << m_bLogOnExit;
  return EZ_SUCCESS;
}

ezResult ezStateMachineState_SendMsg::Deserialize(ezStreamReader& stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));

  stream >> m_MessageDelay;
  stream >> m_bSendMessageOnEnter;
  stream >> m_bSendMessageOnExit;
  stream >> m_bLogOnEnter;
  stream >> m_bLogOnExit;
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

ezStateMachineComponentManager::ezStateMachineComponentManager(ezWorld* pWorld)
  : ezComponentManager<ComponentType, ezBlockStorageType::Compact>(pWorld)
{
  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezStateMachineComponentManager::ResourceEventHandler, this));
}

ezStateMachineComponentManager::~ezStateMachineComponentManager()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezStateMachineComponentManager::ResourceEventHandler, this));
}

void ezStateMachineComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezStateMachineComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void ezStateMachineComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  // reload
  {
    for (auto hComponent : m_ComponentsToReload)
    {
      ezStateMachineComponent* pComponent = nullptr;
      if (TryGetComponent(hComponent, pComponent) && pComponent->IsActive())
      {
        pComponent->InstantiateStateMachine();
      }
    }
    m_ComponentsToReload.Clear();
  }

  // update
  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      ComponentType* pComponent = it;
      if (pComponent->IsActiveAndSimulating())
      {
        pComponent->Update();
      }
    }
  }
}

void ezStateMachineComponentManager::ResourceEventHandler(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<ezStateMachineResource>())
  {
    ezStateMachineResourceHandle hResource((ezStateMachineResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        m_ComponentsToReload.Insert(it->GetHandle());
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezStateMachineComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_StateMachine")),
    EZ_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_StateChangedSender)
  }
  EZ_END_MESSAGESENDERS;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetState, In, "Name"),
  }
  EZ_END_FUNCTIONS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}

EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezStateMachineComponent::ezStateMachineComponent() = default;
ezStateMachineComponent::ezStateMachineComponent(ezStateMachineComponent&& other) = default;
ezStateMachineComponent::~ezStateMachineComponent() = default;
ezStateMachineComponent& ezStateMachineComponent::operator=(ezStateMachineComponent&& other) = default;

void ezStateMachineComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s << m_sInitialState;
}

void ezStateMachineComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s >> m_sInitialState;
}

void ezStateMachineComponent::OnActivated()
{
  SUPER::OnActivated();

  InstantiateStateMachine();
}

void ezStateMachineComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_pStateMachineInstance = nullptr;
}

void ezStateMachineComponent::SetResource(const ezStateMachineResourceHandle& hResource)
{
  if (m_hResource == hResource)
    return;

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

void ezStateMachineComponent::SetResourceFile(const char* szFile)
{
  ezStateMachineResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezStateMachineResource>(szFile);
    ezResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* ezStateMachineComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void ezStateMachineComponent::SetInitialState(const char* szName)
{
  ezHashedString sInitialState;
  sInitialState.Assign(szName);

  if (m_sInitialState == sInitialState)
    return;

  m_sInitialState = sInitialState;

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

bool ezStateMachineComponent::SetState(ezStringView sName)
{
  if (m_pStateMachineInstance != nullptr)
  {
    ezHashedString sStateName;
    sStateName.Assign(sName);

    return m_pStateMachineInstance->SetState(sStateName).Succeeded();
  }

  return false;
}

void ezStateMachineComponent::SendStateChangedMsg(ezMsgStateMachineStateChanged& msg, ezTime delay)
{
  if (delay > ezTime::Zero())
  {
    m_StateChangedSender.PostEventMessage(msg, this, GetOwner(), delay, ezObjectMsgQueueType::NextFrame);
  }
  else
  {
    m_StateChangedSender.SendEventMessage(msg, this, GetOwner());
  }
}

void ezStateMachineComponent::InstantiateStateMachine()
{
  m_pStateMachineInstance = nullptr;

  if (m_hResource.IsValid() == false)
    return;

  ezResourceLock<ezStateMachineResource> pStateMachineResource(m_hResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pStateMachineResource.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Error("Failed to load state machine '{}'", GetResourceFile());
    return;
  }

  m_pStateMachineInstance = pStateMachineResource->CreateInstance(*this);
  m_pStateMachineInstance->SetBlackboard(ezBlackboardComponent::FindBlackboard(GetOwner()));
  m_pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
}

void ezStateMachineComponent::Update()
{
  if (m_pStateMachineInstance != nullptr)
  {
    m_pStateMachineInstance->Update(GetWorld()->GetClock().GetTimeDiff());
  }
}
