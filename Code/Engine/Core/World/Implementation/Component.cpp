#include <Core/PCH.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponent, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Active", IsActive, SetActive)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezUInt16 ezComponent::TYPE_ID = -1;

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

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);

