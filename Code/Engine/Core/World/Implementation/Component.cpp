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
EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction::None, ezOnComponentFinishedAction::DeleteComponent, ezOnComponentFinishedAction::DeleteGameObject)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezOnComponentFinishedAction2, 1)
EZ_ENUM_CONSTANTS(ezOnComponentFinishedAction2::None, ezOnComponentFinishedAction2::DeleteComponent, ezOnComponentFinishedAction2::DeleteGameObject, ezOnComponentFinishedAction2::Restart)
EZ_END_STATIC_REFLECTED_ENUM()

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

bool ezComponent::SendMessage(ezMessage& msg)
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(), GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg))
    return true;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    ezLog::Warning("Component type '{0}' does not have a message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}

bool ezComponent::SendMessage(ezMessage& msg) const
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(), GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg))
    return true;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    ezLog::Warning("(const) Component type '{0}' does not have a CONST message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
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

