#include <Core/PCH.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponent, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Active", IsActive, SetActive)->AddAttributes(new ezDefaultValueAttribute(true)),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezUInt16 ezComponent::TYPE_ID = ezComponent::GetNextTypeId();
ezUInt16 ezComponent::s_uiNextTypeId;

void ezComponent::SetActive(bool bActive)
{
  m_Flags.AddOrRemove(ezObjectFlags::Active, bActive);
}

void ezComponent::Activate()
{
  m_Flags.Add(ezObjectFlags::Active);
}

void ezComponent::Deactivate()
{
  m_Flags.Remove(ezObjectFlags::Active);
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);

