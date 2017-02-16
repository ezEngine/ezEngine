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


void ezComponent::SetActive(bool bActive)
{
  m_ComponentFlags.AddOrRemove(ezObjectFlags::Active, bActive);
}

void ezComponent::Activate()
{
  m_ComponentFlags.Add(ezObjectFlags::Active);
}

void ezComponent::Deactivate()
{
  m_ComponentFlags.Remove(ezObjectFlags::Active);
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


void ezComponent::EnsureInitialized()
{
  if (IsInitializing())
  {
    ezLog::Error("Recursive initialize call is ignored.");
    return;
  }

  if (!IsInitialized())
  {
    m_ComponentFlags.Add(ezObjectFlags::Initializing);

    Initialize();

    m_ComponentFlags.Remove(ezObjectFlags::Initializing);
    m_ComponentFlags.Add(ezObjectFlags::Initialized);
  }
}


void ezComponent::EnsureSimulationStarted()
{
  EnsureInitialized();

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

ezUInt32 ezComponent::GetWorldIndex() const
{
  return GetWorld()->GetIndex();
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);

