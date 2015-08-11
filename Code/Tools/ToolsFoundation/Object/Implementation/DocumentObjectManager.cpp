#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <Foundation/IO/MemoryStream.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEmptyProperties, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEmptyProperties ezDocumentObjectRoot::s_Properties;
ezReflectedTypeDirectAccessor ezDocumentObjectRoot::s_Accessor(&ezDocumentObjectRoot::s_Properties);


ezDocumentObjectManager::ezDocumentObjectManager()
  : m_pDocument(nullptr)
{
}

ezDocumentObjectManager::~ezDocumentObjectManager()
{
  EZ_ASSERT_DEV(m_GuidToObject.IsEmpty(), "Not all objects have been destroyed!");
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Object Construction / Destruction
////////////////////////////////////////////////////////////////////////

ezDocumentObjectBase* ezDocumentObjectManager::CreateObject(const ezRTTI* pRtti, ezUuid guid)
{
  ezDocumentObjectBase* pObject = InternalCreateObject(pRtti);

  if (pObject)
  {
    //if (!pRtti->GetDefaultInitialization().IsEmpty())
    //{
    //  ezMemoryStreamStorage storage;
    //  ezMemoryStreamWriter writer(&storage);
    //  ezMemoryStreamReader reader(&storage);
    //  writer.WriteBytes(hType.GetType()->GetDefaultInitialization().GetData(), hType.GetType()->GetDefaultInitialization().GetElementCount());

    //  ezToolsReflectionUtils::ReadObjectPropertiesFromJSON(reader, pObject->GetTypeAccessor());
    //}

    if (guid.IsValid())
      pObject->m_Guid = guid;
    else
      pObject->m_Guid.CreateNewUuid();
  }

  return pObject;
}

void ezDocumentObjectManager::DestroyObject(ezDocumentObjectBase* pObject)
{
  for (ezDocumentObjectBase* pChild : pObject->m_Children)
  {
    DestroyObject(pChild);
  }

  InternalDestroyObject(pObject);
}

void ezDocumentObjectManager::DestroyAllObjects(ezDocumentObjectManager* pDocumentObjectManager)
{
  for (auto child : m_RootObject.m_Children)
  {
    pDocumentObjectManager->DestroyObject(child);
  }

  m_RootObject.m_Children.Clear();
  m_GuidToObject.Clear();
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Structure Change
////////////////////////////////////////////////////////////////////////

void ezDocumentObjectManager::AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent, ezInt32 iChildIndex)
{
  EZ_ASSERT_DEV(pObject->GetGuid().IsValid(), "Object Guid invalid! Object was not created via an ezObjectManagerBase!");
  EZ_ASSERT_DEV(CanAdd(pObject->GetTypeAccessor().GetType(), pParent), "Trying to execute invalid add!");

  if (pParent == nullptr)
    pParent = &m_RootObject;

  if (iChildIndex < 0)
    iChildIndex = pParent->m_Children.GetCount();

  EZ_ASSERT_DEV((ezUInt32)iChildIndex <= pParent->m_Children.GetCount(), "Child index to add to is out of bounds of the parent's children!");

  ezDocumentObjectStructureEvent e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;
  e.m_uiNewChildIndex = (ezUInt32)iChildIndex;

  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectAdded;
  m_StructureEvents.Broadcast(e);

  pObject->m_pParent = pParent;
  pParent->m_Children.Insert(pObject, (ezUInt32)iChildIndex);

  RecursiveAddGuids(pObject);
 
  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectAdded;
  m_StructureEvents.Broadcast(e);
}

void ezDocumentObjectManager::RemoveObject(ezDocumentObjectBase* pObject)
{
  EZ_ASSERT_DEV(CanRemove(pObject), "Trying to execute invalid remove!");

  ezDocumentObjectStructureEvent e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;

  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved;
  m_StructureEvents.Broadcast(e);

  pObject->m_pParent->m_Children.Remove(pObject);
  pObject->m_pParent = nullptr;

  RecursiveRemoveGuids(pObject);

  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectRemoved;
  m_StructureEvents.Broadcast(e);
}

void ezDocumentObjectManager::MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex)
{
  EZ_ASSERT_DEV(CanMove(pObject, pNewParent, iChildIndex), "Trying to execute invalid move!");

  if (pNewParent == nullptr)
    pNewParent = &m_RootObject;

  if (iChildIndex < 0)
    iChildIndex = pNewParent->m_Children.GetCount();

  EZ_ASSERT_DEV((ezUInt32)iChildIndex <= pNewParent->m_Children.GetCount(), "Child index to insert to is out of bounds of the new parent's children!");

  ezDocumentObjectStructureEvent e;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;
  e.m_uiNewChildIndex = (ezUInt32)iChildIndex;
  e.m_EventType = ezDocumentObjectStructureEvent::Type::BeforeObjectMoved;
  m_StructureEvents.Broadcast(e);


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



  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectMoved;
  m_StructureEvents.Broadcast(e);


  e.m_EventType = ezDocumentObjectStructureEvent::Type::AfterObjectMoved2;
  m_StructureEvents.Broadcast(e);
}

const ezDocumentObjectBase* ezDocumentObjectManager::GetObject(const ezUuid& guid) const
{
  const ezDocumentObjectBase* pObject = nullptr;
  if (m_GuidToObject.TryGetValue(guid, pObject))
  {
    return pObject;
  }

  return nullptr;
}

ezDocumentObjectBase* ezDocumentObjectManager::GetObject(const ezUuid& guid)
{
  return const_cast<ezDocumentObjectBase*>(((const ezDocumentObjectManager*)this)->GetObject(guid));
}


////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Structure Change Test
////////////////////////////////////////////////////////////////////////

bool ezDocumentObjectManager::CanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const
{
  // Test whether parent exists in tree.
  if (pParent == GetRootObject())
    pParent = nullptr;

  if (pParent != nullptr)
  {  
    const ezDocumentObjectBase* pObjectInTree = GetObject(pParent->GetGuid());

    EZ_ASSERT_DEV(pObjectInTree == pParent, "Tree Corruption!!!");

    if (pObjectInTree == nullptr)
      return false;
  }

  return InternalCanAdd(pRtti, pParent);
}

bool ezDocumentObjectManager::CanRemove(const ezDocumentObjectBase* pObject) const
{
  const ezDocumentObjectBase* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return false;

  EZ_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

bool ezDocumentObjectManager::CanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const
{
  if (pNewParent == nullptr)
    pNewParent = GetRootObject();

  if (pObject == pNewParent)
    return false;

  const ezDocumentObjectBase* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return false;

  EZ_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  if (pNewParent != GetRootObject())
  {
    const ezDocumentObjectBase* pNewParentInTree = GetObject(pNewParent->GetGuid());

    if (pNewParentInTree == nullptr)
      return false;

    EZ_ASSERT_DEV(pNewParentInTree == pNewParent, "Tree Corruption!!!");
  }

  const ezDocumentObjectBase* pCurParent = pNewParent->GetParent();

  while (pCurParent)
  {
    if (pCurParent == pObject)
      return false;

    pCurParent = pCurParent->GetParent();
  }

  if (iChildIndex < 0)
    iChildIndex = pNewParent->GetChildren().GetCount();

  if ((ezUInt32)iChildIndex > pNewParent->GetChildren().GetCount())
    return false;

  if (pNewParent == pObject->GetParent())
  {
    // Test whether we are moving before or after ourselves, both of which are not allowed and would not change the tree.
    ezUInt32 iIndex = pObject->GetParent()->GetChildren().IndexOf((ezDocumentObjectBase*) pObject);
    if (iChildIndex == iIndex || iChildIndex == iIndex + 1)
      return false;
  }

  if (pNewParent == GetRootObject())
    pNewParent = nullptr;

  return InternalCanMove(pObject, pNewParent, iChildIndex);
}

////////////////////////////////////////////////////////////////////////
// ezDocumentObjectManager Private Functions
////////////////////////////////////////////////////////////////////////

void ezDocumentObjectManager::RecursiveAddGuids(ezDocumentObjectBase* pObject)
{
  m_GuidToObject[pObject->m_Guid] = pObject;

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveAddGuids(pObject->GetChildren()[c]);
}

void ezDocumentObjectManager::RecursiveRemoveGuids(ezDocumentObjectBase* pObject)
{
  m_GuidToObject.Remove(pObject->m_Guid);

  for (ezUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}
