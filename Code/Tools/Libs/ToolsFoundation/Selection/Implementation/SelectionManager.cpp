#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

ezSelectionManager::ezSelectionManager(const ezDocumentObjectManager* pObjectManager)
{
  auto pStorage = EZ_DEFAULT_NEW(Storage);
  pStorage->m_pObjectManager = pObjectManager;
  SwapStorage(pStorage);
}

ezSelectionManager::~ezSelectionManager()
{
  m_ObjectStructureUnsubscriber.Unsubscribe();
  m_EventsUnsubscriber.Unsubscribe();
}

void ezSelectionManager::TreeEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
      RemoveObject(e.m_pObject, true);
      break;
    default:
      return;
  }
}

bool ezSelectionManager::RecursiveRemoveFromSelection(const ezDocumentObject* pObject)
{
  auto it = m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid());

  bool bRemoved = false;
  if (it.IsValid())
  {
    m_pSelectionStorage->m_SelectionSet.Remove(it);
    m_pSelectionStorage->m_SelectionList.RemoveAndCopy(pObject);
    bRemoved = true;
  }

  for (const ezDocumentObject* pChild : pObject->GetChildren())
  {
    bRemoved = bRemoved || RecursiveRemoveFromSelection(pChild);
  }
  return bRemoved;
}

void ezSelectionManager::Clear()
{
  if (!m_pSelectionStorage->m_SelectionList.IsEmpty() || !m_pSelectionStorage->m_SelectionSet.IsEmpty())
  {
    m_pSelectionStorage->m_SelectionList.Clear();
    m_pSelectionStorage->m_SelectionSet.Clear();

    ezSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject = nullptr;
    e.m_Type = ezSelectionManagerEvent::Type::SelectionCleared;

    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void ezSelectionManager::AddObject(const ezDocumentObject* pObject)
{
  EZ_ASSERT_DEBUG(pObject, "Object must be valid");

  if (IsSelected(pObject))
    return;

  EZ_ASSERT_DEV(pObject->GetDocumentObjectManager() == m_pSelectionStorage->m_pObjectManager, "Passed in object does not belong to same object manager.");
  ezStatus res = m_pSelectionStorage->m_pObjectManager->CanSelect(pObject);
  if (res.m_Result.Failed())
  {
    ezLog::Error("{0}", res.m_sMessage);
    return;
  }

  m_pSelectionStorage->m_SelectionList.PushBack(pObject);
  m_pSelectionStorage->m_SelectionSet.Insert(pObject->GetGuid());

  ezSelectionManagerEvent e;
  e.m_pDocument = GetDocument();
  e.m_pObject = pObject;
  e.m_Type = ezSelectionManagerEvent::Type::ObjectAdded;

  m_pSelectionStorage->m_Events.Broadcast(e);
}

void ezSelectionManager::RemoveObject(const ezDocumentObject* pObject, bool bRecurseChildren)
{
  if (bRecurseChildren)
  {
    // We only want one message for the change in selection so we first everything and then fire
    // SelectionSet instead of multiple ObjectRemoved messages.
    if (RecursiveRemoveFromSelection(pObject))
    {
      ezSelectionManagerEvent e;
      e.m_pDocument = GetDocument();
      e.m_pObject = nullptr;
      e.m_Type = ezSelectionManagerEvent::Type::SelectionSet;
      m_pSelectionStorage->m_Events.Broadcast(e);
    }
  }
  else
  {
    auto it = m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid());

    if (!it.IsValid())
      return;

    m_pSelectionStorage->m_SelectionSet.Remove(it);
    m_pSelectionStorage->m_SelectionList.RemoveAndCopy(pObject);

    ezSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject = pObject;
    e.m_Type = ezSelectionManagerEvent::Type::ObjectRemoved;

    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void ezSelectionManager::SetSelection(const ezDocumentObject* pSingleObject)
{
  ezDeque<const ezDocumentObject*> objs;
  objs.PushBack(pSingleObject);
  SetSelection(objs);
}

void ezSelectionManager::SetSelection(const ezDeque<const ezDocumentObject*>& selection)
{
  if (selection.IsEmpty())
  {
    Clear();
    return;
  }

  if (m_pSelectionStorage->m_SelectionList == selection)
    return;

  m_pSelectionStorage->m_SelectionList.Clear();
  m_pSelectionStorage->m_SelectionSet.Clear();

  m_pSelectionStorage->m_SelectionList.Reserve(selection.GetCount());

  for (ezUInt32 i = 0; i < selection.GetCount(); ++i)
  {
    if (selection[i] != nullptr)
    {
      EZ_ASSERT_DEV(selection[i]->GetDocumentObjectManager() == m_pSelectionStorage->m_pObjectManager, "Passed in object does not belong to same object manager.");
      ezStatus res = m_pSelectionStorage->m_pObjectManager->CanSelect(selection[i]);
      if (res.m_Result.Failed())
      {
        ezLog::Error("{0}", res.m_sMessage);
        continue;
      }
      // actually == nullptr should never happen, unless we have an error somewhere else
      m_pSelectionStorage->m_SelectionList.PushBack(selection[i]);
      m_pSelectionStorage->m_SelectionSet.Insert(selection[i]->GetGuid());
    }
  }

  {
    // Sync selection model.
    ezSelectionManagerEvent e;
    e.m_pDocument = GetDocument();
    e.m_pObject = nullptr;
    e.m_Type = ezSelectionManagerEvent::Type::SelectionSet;
    m_pSelectionStorage->m_Events.Broadcast(e);
  }
}

void ezSelectionManager::ToggleObject(const ezDocumentObject* pObject)
{
  if (IsSelected(pObject))
    RemoveObject(pObject);
  else
    AddObject(pObject);
}

const ezDocumentObject* ezSelectionManager::GetCurrentObject() const
{
  return m_pSelectionStorage->m_SelectionList.IsEmpty() ? nullptr : m_pSelectionStorage->m_SelectionList[m_pSelectionStorage->m_SelectionList.GetCount() - 1];
}

bool ezSelectionManager::IsSelected(const ezDocumentObject* pObject) const
{
  return m_pSelectionStorage->m_SelectionSet.Find(pObject->GetGuid()).IsValid();
}

bool ezSelectionManager::IsParentSelected(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pParent = pObject->GetParent();

  while (pParent != nullptr)
  {
    if (m_pSelectionStorage->m_SelectionSet.Find(pParent->GetGuid()).IsValid())
      return true;

    pParent = pParent->GetParent();
  }

  return false;
}

const ezDocument* ezSelectionManager::GetDocument() const
{
  return m_pSelectionStorage->m_pObjectManager->GetDocument();
}

ezSharedPtr<ezSelectionManager::Storage> ezSelectionManager::SwapStorage(ezSharedPtr<ezSelectionManager::Storage> pNewStorage)
{
  EZ_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pSelectionStorage;

  m_ObjectStructureUnsubscriber.Unsubscribe();
  m_EventsUnsubscriber.Unsubscribe();

  m_pSelectionStorage = pNewStorage;

  m_pSelectionStorage->m_pObjectManager->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSelectionManager::TreeEventHandler, this), m_ObjectStructureUnsubscriber);
  m_pSelectionStorage->m_Events.AddEventHandler([this](const ezSelectionManagerEvent& e)
    { m_Events.Broadcast(e); },
    m_EventsUnsubscriber);

  return retVal;
}

struct ezObjectHierarchyComparor
{
  using Tree = ezHybridArray<const ezDocumentObject*, 4>;
  ezObjectHierarchyComparor(ezDeque<const ezDocumentObject*>& ref_items)
  {
    for (const ezDocumentObject* pObject : ref_items)
    {
      Tree& tree = lookup[pObject];
      while (pObject)
      {
        tree.PushBack(pObject);
        pObject = pObject->GetParent();
      }
      std::reverse(begin(tree), end(tree));
    }
  }

  EZ_ALWAYS_INLINE bool Less(const ezDocumentObject* lhs, const ezDocumentObject* rhs) const
  {
    const Tree& A = *lookup.GetValue(lhs);
    const Tree& B = *lookup.GetValue(rhs);

    const ezUInt32 minSize = ezMath::Min(A.GetCount(), B.GetCount());
    for (ezUInt32 i = 0; i < minSize; i++)
    {
      // The first element in the loop should always be the root so there is not risk that there is no common parent.
      if (A[i] != B[i])
      {
        // These elements are the first different ones so they share the same parent.
        // We just assume that the hierarchy is integer-based for now.
        return A[i]->GetPropertyIndex().ConvertTo<ezUInt32>() < B[i]->GetPropertyIndex().ConvertTo<ezUInt32>();
      }
    }

    return A.GetCount() < B.GetCount();
  }

  EZ_ALWAYS_INLINE bool Equal(const ezDocumentObject* lhs, const ezDocumentObject* rhs) const { return lhs == rhs; }

  ezMap<const ezDocumentObject*, Tree> lookup;
};

const ezDeque<const ezDocumentObject*> ezSelectionManager::GetTopLevelSelection(bool bInOrderOfSceneTree) const
{
  ezDeque<const ezDocumentObject*> items;

  for (const auto* pObj : m_pSelectionStorage->m_SelectionList)
  {
    if (!IsParentSelected(pObj))
    {
      items.PushBack(pObj);
    }
  }

  if (bInOrderOfSceneTree)
  {
    ezObjectHierarchyComparor c(items);
    items.Sort(c);
  }

  return items;
}

const ezDeque<const ezDocumentObject*> ezSelectionManager::GetTopLevelSelection(const ezRTTI* pBase, bool bInOrderOfSceneTree) const
{
  ezDeque<const ezDocumentObject*> items;

  for (const auto* pObj : m_pSelectionStorage->m_SelectionList)
  {
    if (!pObj->GetTypeAccessor().GetType()->IsDerivedFrom(pBase))
      continue;

    if (!IsParentSelected(pObj))
    {
      items.PushBack(pObj);
    }
  }

  if (bInOrderOfSceneTree)
  {
    ezObjectHierarchyComparor c(items);
    items.Sort(c);
  }

  return items;
}
