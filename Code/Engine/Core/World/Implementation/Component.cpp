#include <PCH.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponent, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Active", IsActive, SetActive)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction, 1)
EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction::None, ezOnComponentFinishedAction::DeleteComponent, ezOnComponentFinishedAction::DeleteEntity)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_IMPLEMENT_MESSAGE_TYPE(ezEventMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventMessage, 1, ezRTTIDefaultAllocator<ezEventMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezComponent::SetActive(bool bActive)
{
  if (m_ComponentFlags.IsSet(ezObjectFlags::Active) != bActive)
  {
    m_ComponentFlags.AddOrRemove(ezObjectFlags::Active, bActive);

    if (IsInitialized())
    {
      if (bActive)
      {
        OnActivated();

        if (GetWorld()->GetWorldSimulationEnabled())
        {
          EnsureSimulationStarted();
        }
      }
      else
      {
        OnDeactivated();

        m_ComponentFlags.Remove(ezObjectFlags::SimulationStarted);
      }
    }
  }
}

void ezComponent::Activate()
{
  SetActive(true);
}

void ezComponent::Deactivate()
{
  SetActive(false);
}

ezWorld* ezComponent::GetWorld()
{
  return m_pManager->GetWorld();
}

const ezWorld* ezComponent::GetWorld() const
{
  return m_pManager->GetWorld();
}

ezComponentHandle ezComponent::GetHandle() const
{
  return ezComponentHandle(ezComponentId(m_InternalId, GetTypeId(), GetWorld()->GetIndex()));
}

void ezComponent::SerializeComponent(ezWorldWriter& stream) const
{

}

void ezComponent::DeserializeComponent(ezWorldReader& stream)
{

}

void ezComponent::EnsureInitialized()
{
  EZ_ASSERT_DEV(m_pOwner != nullptr, "Owner must not be null");

  if (IsInitializing())
  {
    ezLog::Error("Recursive initialize call is ignored.");
    return;
  }

  if (!IsInitialized())
  {
    m_pMessageDispatchType = GetDynamicRTTI();

    m_ComponentFlags.Add(ezObjectFlags::Initializing);

    Initialize();

    m_ComponentFlags.Remove(ezObjectFlags::Initializing);
    m_ComponentFlags.Add(ezObjectFlags::Initialized);
  }
}

void ezComponent::EnsureSimulationStarted()
{
  EZ_ASSERT_DEV(IsActiveAndInitialized(), "Must not be called on uninitialized or inactive components.");
  EZ_ASSERT_DEV(GetWorld()->GetWorldSimulationEnabled(), "Must not be called when the world is not simulated.");

  if (m_ComponentFlags.IsSet(ezObjectFlags::SimulationStarting))
  {
    ezLog::Error("Recursive simulation started call is ignored.");
    return;
  }

  if (!IsSimulationStarted())
  {
    m_ComponentFlags.Add(ezObjectFlags::SimulationStarting);

    OnSimulationStarted();

    m_ComponentFlags.Remove(ezObjectFlags::SimulationStarting);
    m_ComponentFlags.Add(ezObjectFlags::SimulationStarted);
  }
}

void ezComponent::PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType)
{
  GetWorld()->PostMessage(GetHandle(), msg, queueType);
}

void ezComponent::PostMessage(ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay)
{
  GetWorld()->PostMessage(GetHandle(), msg, queueType, delay);
}

void ezComponent::Initialize()
{

}

void ezComponent::Deinitialize()
{
  EZ_ASSERT_DEV(m_pOwner != nullptr, "Owner must still be valid");

  if (IsActive())
  {
    Deactivate();
  }
}

void ezComponent::OnActivated()
{

}

void ezComponent::OnDeactivated()
{

}

void ezComponent::OnSimulationStarted()
{

}

void ezComponent::EnableEventHandlerMode(bool enable)
{
  m_ComponentFlags.AddOrRemove(ezObjectFlags::IsEventHandler, enable);

  if (enable)
  {
    // this is an optimization, to know whether the object has any component with this flag
    GetOwner()->m_Flags.Add(ezObjectFlags::IsEventHandler);
  }
  else
  {
    // check whether there is any other component with this flag left

    const auto& components = GetOwner()->GetComponents();
    for (const auto& comp : components)
    {
      // one is left, abort
      if (comp->m_ComponentFlags.IsAnySet(ezObjectFlags::IsEventHandler))
        return;
    }

    // none is left, remove flag
    GetOwner()->m_Flags.Remove(ezObjectFlags::IsEventHandler);
  }
}

void ezComponent::EnableUnhandledMessageHandler(bool enable)
{
  m_ComponentFlags.AddOrRemove(ezObjectFlags::UnhandledMessageHandler, enable);
}

bool ezComponent::OnUnhandledMessage(ezMessage& msg)
{
  return false;
}

bool ezComponent::OnUnhandledMessage(ezMessage& msg) const
{
  return false;
}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);

