#include <CorePCH.h>

#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponent, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezComponent::SetActiveFlag(bool bEnabled)
{
  if (m_ComponentFlags.IsSet(ezObjectFlags::ActiveFlag) != bEnabled)
  {
    m_ComponentFlags.AddOrRemove(ezObjectFlags::ActiveFlag, bEnabled);

    UpdateActiveState(GetOwner() == nullptr ? true : GetOwner()->IsActive());
  }
}

void ezComponent::UpdateActiveState(bool bOwnerActive)
{
  const bool bSelfActive = bOwnerActive && m_ComponentFlags.IsSet(ezObjectFlags::ActiveFlag);

  if (m_ComponentFlags.IsSet(ezObjectFlags::ActiveState) != bSelfActive)
  {
    m_ComponentFlags.AddOrRemove(ezObjectFlags::ActiveState, bSelfActive);

    if (IsInitialized())
    {
      if (bSelfActive)
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

void ezComponent::SerializeComponent(ezWorldWriter& stream) const {}

void ezComponent::DeserializeComponent(ezWorldReader& stream) {}

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

bool ezComponent::SendMessageInternal(ezMessage& msg, bool bWasPostedMsg)
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment",
        msg.GetId(), GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    ezLog::Warning("Component type '{0}' does not have a message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(),
      msg.GetId());
#endif

  return false;
}

bool ezComponent::SendMessageInternal(ezMessage& msg, bool bWasPostedMsg) const
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment",
        msg.GetId(), GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    ezLog::Warning("(const) Component type '{0}' does not have a CONST message handler for messages of type {1}",
      GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}

void ezComponent::PostMessage(const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay) const
{
  GetWorld()->PostMessage(GetHandle(), msg, queueType, delay);
}

void ezComponent::Initialize() {}

void ezComponent::Deinitialize()
{
  EZ_ASSERT_DEV(m_pOwner != nullptr, "Owner must still be valid");

  SetActiveFlag(false);
}

void ezComponent::OnActivated() {}

void ezComponent::OnDeactivated() {}

void ezComponent::OnSimulationStarted() {}

void ezComponent::EnableUnhandledMessageHandler(bool enable)
{
  m_ComponentFlags.AddOrRemove(ezObjectFlags::UnhandledMessageHandler, enable);
}

bool ezComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg)
{
  return false;
}

bool ezComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const
{
  return false;
}

void ezComponent::SetUserFlag(ezUInt8 flagIndex, bool set)
{
  EZ_ASSERT_DEBUG(flagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", flagIndex);

  m_ComponentFlags.AddOrRemove(static_cast<ezObjectFlags::Enum>(ezObjectFlags::UserFlag0 << flagIndex), set);
}

bool ezComponent::GetUserFlag(ezUInt8 flagIndex) const
{
  EZ_ASSERT_DEBUG(flagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", flagIndex);

  return m_ComponentFlags.IsSet(static_cast<ezObjectFlags::Enum>(ezObjectFlags::UserFlag0 << flagIndex));
}
