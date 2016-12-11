#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezSelectionManager::ezSelectionManager()
{
}

void ezSelectionManager::SetOwner(const ezDocument* pDocument)
{
  if (pDocument)
  {
    pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezSelectionManager::TreeEventHandler, this));

  }
  else
  {
    m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezSelectionManager::TreeEventHandler, this));
  }

  m_pDocument = pDocument;
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
  auto it = m_SelectionSet.Find(pObject->GetGuid());

  bool bRemoved = false;
  if (it.IsValid())
  {
    m_SelectionSet.Remove(it);
    m_SelectionList.Remove(pObject);
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
  if (!m_SelectionList.IsEmpty() || !m_SelectionSet.IsEmpty())
  {
    m_SelectionList.Clear();
    m_SelectionSet.Clear();

    ezSelectionManagerEvent e;
    e.m_pDocument = m_pDocument;
    e.m_pObject = nullptr;
    e.m_Type = ezSelectionManagerEvent::Type::SelectionCleared;

    m_Events.Broadcast(e);
  }
}

void ezSelectionManager::AddObject(const ezDocumentObject* pObject)
{
  if (IsSelected(pObject))
    return;

  ezStatus res = GetDocument()->GetObjectManager()->CanSelect(pObject);
  if (res.m_Result.Failed())
  {
    ezLog::ErrorPrintf("%s", res.m_sMessage.GetData());
    return;
  }

  m_SelectionList.PushBack(pObject);
  m_SelectionSet.Insert(pObject->GetGuid());

  ezSelectionManagerEvent e;
  e.m_pDocument = m_pDocument;
  e.m_pObject = pObject;
  e.m_Type = ezSelectionManagerEvent::Type::ObjectAdded;

  m_Events.Broadcast(e);
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
      e.m_pDocument = m_pDocument;
      e.m_pObject = nullptr;
      e.m_Type = ezSelectionManagerEvent::Type::SelectionSet;
      m_Events.Broadcast(e);
    }
  }
  else
  {
    auto it = m_SelectionSet.Find(pObject->GetGuid());

    if (!it.IsValid())
      return;

    m_SelectionSet.Remove(it);
    m_SelectionList.Remove(pObject);

    ezSelectionManagerEvent e;
    e.m_pDocument = m_pDocument;
    e.m_pObject = pObject;
    e.m_Type = ezSelectionManagerEvent::Type::ObjectRemoved;

    m_Events.Broadcast(e);
  }
}

void ezSelectionManager::SetSelection(const ezDeque<const ezDocumentObject*>& Selection)
{
  if (Selection.IsEmpty())
  {
    Clear();
    return;
  }

  m_SelectionList.Clear();
  m_SelectionSet.Clear();

  m_SelectionList.Reserve(Selection.GetCount());

  for (ezUInt32 i = 0; i < Selection.GetCount(); ++i)
  {
    if (Selection[i] != nullptr)
    {
      ezStatus res = GetDocument()->GetObjectManager()->CanSelect(Selection[i]);
      if (res.m_Result.Failed())
      {
        ezLog::ErrorPrintf("%s", res.m_sMessage.GetData());
        continue;
      }
      // actually == nullptr should never happen, unless we have an error somewhere else
      m_SelectionList.PushBack(Selection[i]);
      m_SelectionSet.Insert(Selection[i]->GetGuid());
    }
  }

  ezSelectionManagerEvent e;
  e.m_pDocument = m_pDocument;
  e.m_pObject = nullptr;
  e.m_Type = ezSelectionManagerEvent::Type::SelectionSet;

  m_Events.Broadcast(e);
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
  return m_SelectionList.IsEmpty() ? nullptr : m_SelectionList[m_SelectionList.GetCount() - 1];
}

bool ezSelectionManager::IsSelected(const ezDocumentObject* pObject) const
{
  return m_SelectionSet.Find(pObject->GetGuid()).IsValid();
}

bool ezSelectionManager::IsParentSelected(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pParent = pObject->GetParent();

  while (pParent != nullptr)
  {
    if (m_SelectionSet.Find(pParent->GetGuid()).IsValid())
      return true;

    pParent = pParent->GetParent();
  }

  return false;
}

const ezDeque<const ezDocumentObject*> ezSelectionManager::GetTopLevelSelection() const
{
  ezDeque<const ezDocumentObject*> items;

  for (const auto* pObj : m_SelectionList)
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

  for (const auto* pObj : m_SelectionList)
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
