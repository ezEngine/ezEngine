#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEmptyProperties, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEmptyProperties ezDocumentObjectRoot::s_Properties;
ezReflectedTypeDirectAccessor ezDocumentObjectRoot::s_Accessor(&ezDocumentObjectRoot::s_Properties);

void ezDocumentObjectTree::AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent)
{
  if (pParent == nullptr)
    pParent = &m_RootObject;

  pObject->m_pParent = pParent;
  pParent->m_Children.PushBack(pObject);

  m_GuidToObject[pObject->m_Guid] = pObject;

  Event e;
  e.m_EventType = Event::Type::ObjectAdded;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;
  
  m_Events.Broadcast(e);
}

void ezDocumentObjectTree::RemoveObject(ezDocumentObjectBase* pObject)
{
  Event e;
  e.m_EventType = Event::Type::ObjectRemoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;
  
  pObject->m_pParent->m_Children.Remove(pObject);
  pObject->m_pParent = nullptr;

  m_GuidToObject.Erase(pObject->m_Guid);

  m_Events.Broadcast(e);
}

void ezDocumentObjectTree::MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent)
{
  if (pNewParent == nullptr)
    pNewParent = &m_RootObject;

  Event e;
  e.m_EventType = Event::Type::ObjectMoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;
  
  pObject->m_pParent->m_Children.Remove(pObject);
  pObject->m_pParent = pNewParent;
  pNewParent->m_Children.PushBack(pObject);

  m_Events.Broadcast(e);
}

const ezDocumentObjectBase* ezDocumentObjectTree::GetObject(const ezUuid& guid) const
{
  auto it = m_GuidToObject.Find(guid);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}
