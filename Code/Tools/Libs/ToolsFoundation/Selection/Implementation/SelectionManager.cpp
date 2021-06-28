#include <ToolsFoundationPCH.h>

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Selection/SelectionManager.h>

ezSelectionManager::ezSelectionManager()
{
  auto pStorage = EZ_DEFAULT_NEW(Storage);
  SwapStorage(pStorage);
}

void ezSelectionManager::SetOwner(const ezDocumentObjectManager* pObjectManager)
{
  EZ_ASSERT_DEV((m_pObjectManager == nullptr) != (pObjectManager == nullptr), "SetOwner can only be called once.");
  if (pObjectManager)
  {
    pObjectManager->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSelectionManager::TreeEventHandler, this));
  }
  else
  {
    m_pObjectManager->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSelectionManager::TreeEventHandler, this));
  }

  m_pObjectManager = pObjectManager;
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

  ezStatus res = m_pObjectManager->CanSelect(pObject);
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

void ezSelectionManager::SetSelection(const ezDeque<const ezDocumentObject*>& Selection)
{
  if (Selection.IsEmpty())
  {
    Clear();
    return;
  }

  m_pSelectionStorage->m_SelectionList.Clear();
  m_pSelectionStorage->m_SelectionSet.Clear();

  m_pSelectionStorage->m_SelectionList.Reserve(Selection.GetCount());

  for (ezUInt32 i = 0; i < Selection.GetCount(); ++i)
  {
    if (Selection[i] != nullptr)
    {
      ezStatus res = m_pObjectManager->CanSelect(Selection[i]);
      if (res.m_Result.Failed())
      {
        ezLog::Error("{0}", res.m_sMessage);
        continue;
      }
      // actually == nullptr should never happen, unless we have an error somewhere else
      m_pSelectionStorage->m_SelectionList.PushBack(Selection[i]);
      m_pSelectionStorage->m_SelectionSet.Insert(Selection[i]->GetGuid());
    }
  }

  ezSelectionManagerEvent e;
  e.m_pDocument = GetDocument();
  e.m_pObject = nullptr;
  e.m_Type = ezSelectionManagerEvent::Type::SelectionSet;

  m_pSelectionStorage->m_Events.Broadcast(e);
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
  return m_pObjectManager->GetDocument();
}

ezSharedPtr<ezSelectionManager::Storage> ezSelectionManager::SwapStorage(ezSharedPtr<ezSelectionManager::Storage> pNewStorage)
{
  EZ_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pSelectionStorage;

  m_EventsUnsubscriber.Clear();

  m_pSelectionStorage = pNewStorage;

  m_pSelectionStorage->m_Events.AddEventHandler([this](const ezSelectionManagerEvent& e) { m_Events.Broadcast(e); }, m_EventsUnsubscriber);

  return retVal;
}

const ezDeque<const ezDocumentObject*> ezSelectionManager::GetTopLevelSelection() const
{
  ezDeque<const ezDocumentObject*> items;

  for (const auto* pObj : m_pSelectionStorage->m_SelectionList)
  {
    if (!IsParentSelected(pObj))
    {
      items.PushBack(pObj);
    }
  }

  return items;
}

const ezDeque<const ezDocumentObject*> ezSelectionManager::GetTopLevelSelection(const ezRTTI* pBase) const
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

  return items;
}
