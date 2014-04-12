#include <Core/PCH.h>
#include <Core/World/GameObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponent, ezReflectedClass, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezUInt16 ezComponent::TYPE_ID = ezComponentManagerBase::GetNextTypeId();

void ezComponent::Activate()
{
}

void ezComponent::Deactivate()
{
}

ezResult ezComponent::Initialize() 
{ 
  return EZ_SUCCESS;
}

ezResult ezComponent::Deinitialize()
{
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);

