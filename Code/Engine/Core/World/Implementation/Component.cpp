#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponent, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(IsActive),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsActiveAndInitialized),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsActiveAndSimulating),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOwner),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_GetWorld),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetUniqueID),
    EZ_SCRIPT_FUNCTION_PROPERTY(DeleteComponent),
    EZ_SCRIPT_FUNCTION_PROPERTY(Initialize)->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezComponent_ScriptBaseClassFunctions::Initialize)),
    EZ_SCRIPT_FUNCTION_PROPERTY(Deinitialize)->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezComponent_ScriptBaseClassFunctions::Deinitialize)),
    EZ_SCRIPT_FUNCTION_PROPERTY(OnActivated)->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezComponent_ScriptBaseClassFunctions::OnActivated)),
    EZ_SCRIPT_FUNCTION_PROPERTY(OnDeactivated)->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezComponent_ScriptBaseClassFunctions::OnDeactivated)),
    EZ_SCRIPT_FUNCTION_PROPERTY(OnSimulationStarted)->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezComponent_ScriptBaseClassFunctions::OnSimulationStarted)),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_Update, In, "DeltaTime")->AddAttributes(new ezScriptBaseClassFunctionAttribute(ezComponent_ScriptBaseClassFunctions::Update)),
  }
  EZ_END_FUNCTIONS;
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

ezWorld* ezComponent::GetWorld()
{
  return m_pManager->GetWorld();
}

const ezWorld* ezComponent::GetWorld() const
{
  return m_pManager->GetWorld();
}

void ezComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  EZ_IGNORE_UNUSED(inout_stream);
}

void ezComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  EZ_IGNORE_UNUSED(inout_stream);
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

void ezComponent::PostMessage(const ezMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

bool ezComponent::HandlesMessage(const ezMessage& msg) const
{
  return m_pMessageDispatchType->CanHandleMessage(msg.GetId());
}

void ezComponent::SetUserFlag(ezUInt8 uiFlagIndex, bool bSet)
{
  EZ_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  m_ComponentFlags.AddOrRemove(static_cast<ezObjectFlags::Enum>(ezObjectFlags::UserFlag0 << uiFlagIndex), bSet);
}

bool ezComponent::GetUserFlag(ezUInt8 uiFlagIndex) const
{
  EZ_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  return m_ComponentFlags.IsSet(static_cast<ezObjectFlags::Enum>(ezObjectFlags::UserFlag0 << uiFlagIndex));
}

void ezComponent::DeleteComponent()
{
  GetOwningManager()->DeleteComponent(this);
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
  EZ_IGNORE_UNUSED(msg);
  EZ_IGNORE_UNUSED(bWasPostedMsg);
  return false;
}

bool ezComponent::OnUnhandledMessage(ezMessage& msg, bool bWasPostedMsg) const
{
  EZ_IGNORE_UNUSED(msg);
  EZ_IGNORE_UNUSED(bWasPostedMsg);
  return false;
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
        // Don't call OnActivated & EnsureSimulationStarted here since there might be other components
        // that are needed in the OnSimulation callback but are activated right after this component.
        // Instead add the component to the initialization batch again.
        // There initialization will be skipped since the component is already initialized.
        GetWorld()->AddComponentToInitialize(GetHandle());
      }
      else
      {
        OnDeactivated();

        m_ComponentFlags.Remove(ezObjectFlags::SimulationStarted);
      }
    }
  }
}

ezGameObject* ezComponent::Reflection_GetOwner() const
{
  return m_pOwner;
}

ezWorld* ezComponent::Reflection_GetWorld() const
{
  return m_pManager->GetWorld();
}

void ezComponent::Reflection_Update(ezTime deltaTime)
{
  EZ_IGNORE_UNUSED(deltaTime);
  // This is just a dummy function for the scripting reflection
}

bool ezComponent::SendMessageInternal(ezMessage& msg, bool bWasPostedMsg)
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
        GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    ezLog::Warning("Component type '{0}' does not have a message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}

bool ezComponent::SendMessageInternal(ezMessage& msg, bool bWasPostedMsg) const
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      ezLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
        GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(ezObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    ezLog::Warning(
      "(const) Component type '{0}' does not have a CONST message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);
