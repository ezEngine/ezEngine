#include <Core/PCH.h>
#include <Core/World/ComponentManager.h>

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

void ezComponent::OnMessage(ezMessage& msg) 
{
}
