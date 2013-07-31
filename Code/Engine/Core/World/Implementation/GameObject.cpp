#include <Core/PCH.h>
#include <Core/World/World.h>

void ezGameObject::ChildIterator::Next()
{
  m_pObject = m_pObject->m_pWorld->GetObject(m_pObject->m_NextSibling);
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
  return ChildIterator(m_pWorld->GetObject(m_FirstChild));
}

ezResult ezGameObject::AddComponent(const ezComponentHandle& component)
{
  if (ezComponent* pComponent = m_pWorld->GetComponent(component))
  {
    pComponent->m_pOwner = this;
    if (pComponent->Initialize() == EZ_SUCCESS)
    {
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
  
  if (ezComponent* pComponent = m_pWorld->GetComponent(component))
  {
    result = pComponent->Deinitialize();
    pComponent->m_pOwner = NULL;
  }

  m_Components.RemoveAtSwap(uiIndex);
  
  return result;
}

void ezGameObject::OnMessage(ezMessage& msg, MsgRouting::Enum routing)
{
  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    if (ezComponent* pComponent = m_pWorld->GetComponent(m_Components[i]))
    {
      pComponent->OnMessage(msg);
    }
  }

  if ((routing & MsgRouting::ToParent) != 0)
  {
    if (ezGameObject* pParent = m_pWorld->GetObject(m_Parent))
    {
      pParent->OnMessage(msg, routing);
    }
  }
  if (routing & MsgRouting::ToChildren)
  {
    for (ChildIterator it = GetChildren(); it.IsValid(); ++it)
    {
      it->OnMessage(msg, routing);
    }
  }
}
