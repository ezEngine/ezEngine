#include <Core/PCH.h>
#include <Core/World/World.h>

void ezGameObject::ChildIterator::Next()
{
  m_pObject->m_pWorld->TryGetObject(m_pObject->m_NextSibling, m_pObject);
}

void ezGameObject::operator=(const ezGameObject& other)
{
  EZ_ASSERT(m_pWorld == other.m_pWorld, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags = other.m_Flags;
  m_uiPersistentId = other.m_uiPersistentId;

  m_Parent = other.m_Parent;
  m_FirstChild = other.m_FirstChild;
  m_NextSibling = other.m_NextSibling;

  if (m_pTransformationData != other.m_pTransformationData)
  {
    m_uiHierarchyLevel = other.m_uiHierarchyLevel;
    m_uiTransformationDataIndex = other.m_uiTransformationDataIndex;  
    ezMemoryUtils::Copy(m_pTransformationData, other.m_pTransformationData, 1);
  }

  m_pTransformationData->m_pObject = this;

  m_Components = other.m_Components;
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = NULL;
    if (m_pWorld->TryGetComponent(m_Components[i], pComponent))
    {
      EZ_ASSERT(pComponent->m_pOwner == &other, "");
      pComponent->m_pOwner = this;
    }
  }
}

void ezGameObject::SetName(const char* szName)
{
  m_pWorld->SetObjectName(m_InternalId, szName);
}

const char* ezGameObject::GetName() const
{
  return m_pWorld->GetObjectName(m_InternalId);
}

ezGameObject::ChildIterator ezGameObject::GetChildren() const
{
  ezGameObject* pFirstChild = NULL;
  m_pWorld->TryGetObject(m_FirstChild, pFirstChild);
  return ChildIterator(pFirstChild);
}

ezResult ezGameObject::AddComponent(const ezComponentHandle& component)
{
  ezComponent* pComponent = NULL;
  if (m_pWorld->TryGetComponent(component, pComponent))
  {
    EZ_ASSERT(pComponent->m_pOwner != this, "Component must not be added twice.");

    pComponent->m_pOwner = this;
    if (pComponent->Initialize() == EZ_SUCCESS)
    {
      EZ_ASSERT(IsDynamic() || !pComponent->IsDynamic(), 
        "Cannot attach a dynamic component to a static object. Call MakeDynamic() first.");

      m_Components.PushBack(component);
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

ezResult ezGameObject::RemoveComponent(const ezComponentHandle& component)
{
  ezUInt32 uiIndex = m_Components.IndexOf(component);
  if (uiIndex == ezInvalidIndex)
    return EZ_FAILURE;

  ezResult result = EZ_FAILURE;
  
  ezComponent* pComponent = NULL;
  if (m_pWorld->TryGetComponent(component, pComponent))
  {
    result = pComponent->Deinitialize();
    pComponent->m_pOwner = NULL;
  }

  m_Components.RemoveAtSwap(uiIndex);
  
  return result;
}

bool ezGameObject::TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent) const
{
  return m_pWorld->TryGetComponent(component, out_pComponent);
}

void ezGameObject::SendMessage(ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing /*= MsgRouting::Default*/)
{
  if (routing.IsAnySet(ezObjectMsgRouting::QueuedForPostAsync | ezObjectMsgRouting::QueuedForNextFrame))
  {
    m_pWorld->QueueMessage(GetHandle(), msg, routing);
  }
  else
  {
    m_pWorld->HandleMessage(this, msg, routing);
  }
}

void ezGameObject::OnMessage(ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing)
{
  EZ_ASSERT(!routing.IsAnySet(ezObjectMsgRouting::QueuedForPostAsync | ezObjectMsgRouting::QueuedForNextFrame),
    "Queued message is handled directly");

  // prevent double message handling when sending to parent and children
  const ezUInt32 uiHandledMessageCounter = m_pWorld->GetHandledMessageCounter();
  if (m_uiHandledMessageCounter == uiHandledMessageCounter)
    return;

  m_uiHandledMessageCounter = uiHandledMessageCounter;

  // always send message to all components
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = NULL;
    if (m_pWorld->TryGetComponent(m_Components[i], pComponent))
    {
      pComponent->OnMessage(msg);
    }
  }

  // route message to parent and/or children
  if (routing.IsSet(ezObjectMsgRouting::ToParent))
  {
    ezGameObject* pParent = NULL;
    if (m_pWorld->TryGetObject(m_Parent, pParent))
    {
      pParent->OnMessage(msg, routing);
    }
  }
  if (routing.IsSet(ezObjectMsgRouting::ToChildren))
  {
    for (ChildIterator it = GetChildren(); it.IsValid(); ++it)
    {
      it->OnMessage(msg, routing);
    }
  }
}


EZ_STATICLINK_FILE(Core, Core_World_Implementation_GameObject);

