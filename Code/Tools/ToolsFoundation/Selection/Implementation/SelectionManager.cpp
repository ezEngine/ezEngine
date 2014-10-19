#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <ToolsFoundation/Document/Document.h>

ezSelectionManager::ezSelectionManager()
{
}

void ezSelectionManager::SetOwner(const ezDocumentBase* pDocument)
{
  if (pDocument)
  {
    pDocument->GetObjectTree()->m_Events.AddEventHandler(ezDelegate<void (const ezDocumentObjectTreeEvent&)>(&ezSelectionManager::TreeEventHandler, this));

  }
  else
  {
    m_pDocument->GetObjectTree()->m_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentObjectTreeEvent&)>(&ezSelectionManager::TreeEventHandler, this));
  }

  m_pDocument = pDocument;
}

void ezSelectionManager::TreeEventHandler(const ezDocumentObjectTreeEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectTreeEvent::Type::BeforeObjectRemoved:
    RemoveObject(e.m_pObject);
    break;
  }
}

void ezSelectionManager::Clear()
{
  m_SelectionList.Clear();
  m_SelectionSet.Clear();

  Event e;
  e.m_pObject = nullptr;
  e.m_Type = Event::Type::SelectionCleared;
  
  m_Events.Broadcast(e);
}

void ezSelectionManager::AddObject(const ezDocumentObjectBase* pObject)
{
  if (IsSelected(pObject))
    return;

  m_SelectionList.PushBack(pObject);
  m_SelectionSet.Insert(pObject->GetGuid());

  Event e;
  e.m_pObject = pObject;
  e.m_Type = Event::Type::ObjectAdded;
  
  m_Events.Broadcast(e);
}

void ezSelectionManager::RemoveObject(const ezDocumentObjectBase* pObject)
{
  auto it = m_SelectionSet.Find(pObject->GetGuid());
  
  if (!it.IsValid())
    return;

  m_SelectionSet.Erase(it);

  m_SelectionList.Remove(pObject);

  Event e;
  e.m_pObject = pObject;
  e.m_Type = Event::Type::ObjectRemoved;
  
  m_Events.Broadcast(e);

}

void ezSelectionManager::SetSelection(const ezDeque<const ezDocumentObjectBase*>& Selection)
{
  if (Selection.IsEmpty())
  {
    Clear();
    return;
  }

  m_SelectionList.Clear();
  m_SelectionSet.Clear();

  m_SelectionList = Selection;

  for (ezUInt32 i = 0; i < m_SelectionList.GetCount(); ++i)
  {
    m_SelectionSet.Insert(m_SelectionList[i]->GetGuid());
  }

  Event e;
  e.m_pObject = nullptr;
  e.m_Type = Event::Type::SelectionSet;
  
  m_Events.Broadcast(e);
}

void ezSelectionManager::ToggleObject(const ezDocumentObjectBase* pObject)
{
  if (IsSelected(pObject))
    RemoveObject(pObject);
  else
    AddObject(pObject);
}

bool ezSelectionManager::IsSelected(const ezDocumentObjectBase* pObject) const
{
  return m_SelectionSet.Find(pObject->GetGuid()).IsValid();
}

