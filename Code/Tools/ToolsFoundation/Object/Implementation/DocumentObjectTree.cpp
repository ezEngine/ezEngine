#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEmptyProperties, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEmptyProperties ezDocumentObjectRoot::s_Properties;
ezReflectedTypeDirectAccessor ezDocumentObjectRoot::s_Accessor(&ezDocumentObjectRoot::s_Properties);

ezDocumentObjectTree::ezDocumentObjectTree(ezDocumentObjectManagerBase* pObjectManager) :
  m_pObjectManager(pObjectManager)
{

}

void ezDocumentObjectTree::RecursiveAddGuids(ezDocumentObjectBase* pObject)
{
  m_GuidToObject[pObject->m_Guid] = pObject;

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveAddGuids(pObject->GetChildren()[c]);
}

void ezDocumentObjectTree::RecursiveRemoveGuids(ezDocumentObjectBase* pObject)
{
  m_GuidToObject.Erase(pObject->m_Guid);

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}

void ezDocumentObjectTree::AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent)
{
  if (pParent == nullptr)
    pParent = &m_RootObject;

  Event e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;

  e.m_EventType = Event::Type::BeforeObjectAdded;
  m_Events.Broadcast(e);

  pObject->m_pParent = pParent;
  pParent->m_Children.PushBack(pObject);

  RecursiveAddGuids(pObject);
 
  e.m_EventType = Event::Type::AfterObjectAdded;
  m_Events.Broadcast(e);
}

void ezDocumentObjectTree::RemoveObject(ezDocumentObjectBase* pObject)
{
  Event e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;

  e.m_EventType = Event::Type::BeforeObjectRemoved;
  m_Events.Broadcast(e);

  pObject->m_pParent->m_Children.Remove(pObject);
  pObject->m_pParent = nullptr;

  RecursiveRemoveGuids(pObject);

  e.m_EventType = Event::Type::AfterObjectRemoved;
  m_Events.Broadcast(e);
}

void ezDocumentObjectTree::MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent)
{
  if (pNewParent == nullptr)
    pNewParent = &m_RootObject;

  if (pNewParent == pObject->m_pParent)
    return;

  Event e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;

  e.m_EventType = Event::Type::BeforeObjectMoved;
  m_Events.Broadcast(e);

  pObject->m_pParent->m_Children.Remove(pObject);
  pObject->m_pParent = pNewParent;
  pNewParent->m_Children.PushBack(pObject);

  e.m_EventType = Event::Type::AfterObjectMoved;
  m_Events.Broadcast(e);
}

const ezDocumentObjectBase* ezDocumentObjectTree::GetObject(const ezUuid& guid) const
{
  auto it = m_GuidToObject.Find(guid);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}
