#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <ToolsFoundation/Document/Document.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEmptyProperties, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEmptyProperties ezDocumentObjectRoot::s_Properties;
ezReflectedTypeDirectAccessor ezDocumentObjectRoot::s_Accessor(&ezDocumentObjectRoot::s_Properties);

ezDocumentObjectTree::ezDocumentObjectTree(const ezDocumentBase* pDocument) :
  m_pDocument(pDocument)
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
  m_GuidToObject.Remove(pObject->m_Guid);

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}

void ezDocumentObjectTree::AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent, ezInt32 iChildIndex)
{
  EZ_ASSERT(pObject->GetGuid().IsValid(), "Object Guid invalid! Object was not created via an ezObjectManagerBase!");
  EZ_ASSERT(m_pDocument->GetObjectManager()->CanAdd(pObject->GetTypeAccessor().GetReflectedTypeHandle(), pParent), "Trying to execute invalid add!");

  if (pParent == nullptr)
    pParent = &m_RootObject;

  if (iChildIndex < 0)
    iChildIndex = pParent->m_Children.GetCount();

  EZ_ASSERT((ezUInt32)iChildIndex <= pParent->m_Children.GetCount(), "Child index to add to is out of bounds of the parent's children!");

  ezDocumentObjectTreeEvent e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;
  e.m_uiNewChildIndex = (ezUInt32)iChildIndex;

  e.m_EventType = ezDocumentObjectTreeEvent::Type::BeforeObjectAdded;
  m_Events.Broadcast(e);

  pObject->m_pParent = pParent;
  pParent->m_Children.Insert(pObject, (ezUInt32)iChildIndex);

  RecursiveAddGuids(pObject);
 
  e.m_EventType = ezDocumentObjectTreeEvent::Type::AfterObjectAdded;
  m_Events.Broadcast(e);
}

void ezDocumentObjectTree::RemoveObject(ezDocumentObjectBase* pObject)
{
  EZ_ASSERT(m_pDocument->GetObjectManager()->CanRemove(pObject), "Trying to execute invalid remove!");

  ezDocumentObjectTreeEvent e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;

  e.m_EventType = ezDocumentObjectTreeEvent::Type::BeforeObjectRemoved;
  m_Events.Broadcast(e);

  pObject->m_pParent->m_Children.Remove(pObject);
  pObject->m_pParent = nullptr;

  RecursiveRemoveGuids(pObject);

  e.m_EventType = ezDocumentObjectTreeEvent::Type::AfterObjectRemoved;
  m_Events.Broadcast(e);
}

void ezDocumentObjectTree::MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex)
{
  EZ_ASSERT(m_pDocument->GetObjectManager()->CanMove(pObject, pNewParent, iChildIndex), "Trying to execute invalid move!");

  if (pNewParent == nullptr)
    pNewParent = &m_RootObject;

  if (iChildIndex < 0)
    iChildIndex = pNewParent->m_Children.GetCount();

  EZ_ASSERT((ezUInt32)iChildIndex <= pNewParent->m_Children.GetCount(), "Child index to insert to is out of bounds of the new parent's children!");

  ezDocumentObjectTreeEvent e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;
  e.m_uiNewChildIndex = (ezUInt32)iChildIndex;
  e.m_EventType = ezDocumentObjectTreeEvent::Type::BeforeObjectMoved;
  m_Events.Broadcast(e);


  if (pNewParent == pObject->m_pParent)
  {
    // Move after oneself?
    ezInt32 iIndex = pNewParent->m_Children.IndexOf(pObject);
    if (iChildIndex > iIndex)
    {
      iChildIndex -= 1;
    }
  }

  pObject->m_pParent->m_Children.Remove(pObject);
  pObject->m_pParent = pNewParent;

  pNewParent->m_Children.Insert(pObject, iChildIndex);



  e.m_EventType = ezDocumentObjectTreeEvent::Type::AfterObjectMoved;
  m_Events.Broadcast(e);
}

const ezDocumentObjectBase* ezDocumentObjectTree::GetObject(const ezUuid& guid) const
{
  auto it = m_GuidToObject.Find(guid);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

ezDocumentObjectBase* ezDocumentObjectTree::GetObject(const ezUuid& guid)
{
  return const_cast<ezDocumentObjectBase*>(((const ezDocumentObjectTree*)this)->GetObject(guid));
}