#include <Core/PCH.h>
#include <Core/World/GameObject.h>

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

void ezComponent::SendMessageToOwner(ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing /*= ezObjectMsgRouting::Default*/)
{
  m_pOwner->SendMessage(msg, routing);
}

void ezComponent::OnMessage(ezMessage& msg) 
{
}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_Component);

