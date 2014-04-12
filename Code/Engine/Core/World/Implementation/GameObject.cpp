#include <Core/PCH.h>
#include <Core/World/World.h>

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGameObject, ezNoBase, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_ACCESSOR_PROPERTY("Position", GetLocalPosition, SetLocalPosition),
    EZ_ACCESSOR_PROPERTY("Rotation", GetLocalRotation, SetLocalRotation),
    EZ_ACCESSOR_PROPERTY("Scaling", GetLocalScaling, SetLocalScaling),
  EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

void ezGameObject::ChildIterator::Next()
{
  m_pObject = m_pObject->m_pWorld->GetObjectUnchecked(m_pObject->m_NextSiblingIndex);
}

void ezGameObject::operator=(const ezGameObject& other)
{
  EZ_ASSERT(m_pWorld == other.m_pWorld, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags = other.m_Flags;

  m_ParentIndex = other.m_ParentIndex;
  m_FirstChildIndex = other.m_FirstChildIndex;
  m_LastChildIndex = other.m_LastChildIndex;
 
  m_NextSiblingIndex = other.m_NextSiblingIndex;
  m_PrevSiblingIndex = other.m_PrevSiblingIndex;
  m_ChildCount = other.m_ChildCount;

  m_uiHandledMessageCounter = other.m_uiHandledMessageCounter;

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
    ezComponent* pComponent = nullptr;
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

void ezGameObject::SetParent(const ezGameObjectHandle& parent)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezGameObject* ezGameObject::GetParent() const
{
  return m_pWorld->GetObjectUnchecked(m_ParentIndex);
}

ezGameObject::ChildIterator ezGameObject::GetChildren() const
{
  return ChildIterator(m_pWorld->GetObjectUnchecked(m_FirstChildIndex));
}

ezResult ezGameObject::AddComponent(const ezComponentHandle& component)
{
  ezComponent* pComponent = nullptr;
  if (m_pWorld->TryGetComponent(component, pComponent))
  {
    EZ_ASSERT(pComponent->m_pOwner == nullptr, "Component must not be added twice.");

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
  
  ezComponent* pComponent = nullptr;
  if (m_pWorld->TryGetComponent(component, pComponent))
  {
    result = pComponent->Deinitialize();
    pComponent->m_pOwner = nullptr;
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
  m_pWorld->HandleMessage(this, msg, routing);
}

void ezGameObject::PostMessage(ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing, 
  ezObjectMsgQueueType::Enum queueType, float fDelay /*= 0.0f*/)
{
  m_pWorld->PostMessage(GetHandle(), msg, routing, queueType, fDelay);
}

void ezGameObject::OnMessage(ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing)
{
  // prevent double message handling when sending to parent and children
  const ezUInt32 uiHandledMessageCounter = m_pWorld->GetHandledMessageCounter();
  if (m_uiHandledMessageCounter == uiHandledMessageCounter)
    return;

  m_uiHandledMessageCounter = uiHandledMessageCounter;

  // always send message to all components
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    ezComponent* pComponent = nullptr;
    if (m_pWorld->TryGetComponent(m_Components[i], pComponent))
    {
      pComponent->OnMessage(msg);
    }
  }

  // route message to parent and/or children
  if (routing.IsSet(ezObjectMsgRouting::ToParent))
  {
    if (ezGameObject* pParent = GetParent())
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

