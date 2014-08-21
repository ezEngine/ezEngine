#include <Core/PCH.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponent, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezUInt16 ezComponent::TYPE_ID = ezComponent::GetNextTypeId();
ezUInt16 ezComponent::s_uiNextTypeId;

void ezComponent::Activate()
{
  m_Flags.Add(ezObjectFlags::Active);
}

void ezComponent::Deactivate()
{
  m_Flags.Remove(ezObjectFlags::Active);
}

ezResult ezComponent::Initialize() 
{ 
  return EZ_SUCCESS;
}

ezResult ezComponent::Deinitialize()
{
  return EZ_SUCCESS;
}

ezResult ezComponent::OnAttachedToObject()
{
  return EZ_SUCCESS;
}

ezResult ezComponent::OnDetachedFromObject()
{
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);

