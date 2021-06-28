#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezDocument;
struct ezDocumentObjectStructureEvent;

struct ezSelectionManagerEvent
{
  enum class Type
  {
    SelectionCleared,
    SelectionSet,
    ObjectAdded,
    ObjectRemoved,
  };

  Type m_Type;
  const ezDocument* m_pDocument;
  const ezDocumentObject* m_pObject;
};

class EZ_TOOLSFOUNDATION_DLL ezSelectionManager
{
public:
  ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&> m_Events;

  // \brief Storage for the command history so it can be swapped when using multiple sub documents
  class Storage : public ezRefCounted
  {
  public:
    ezDeque<const ezDocumentObject*> m_SelectionList;
    ezSet<ezUuid> m_SelectionSet;

    ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&> m_Events;
  };

public:
  ezSelectionManager();

  // Should be called after construction. Can only be called once.
  void SetOwner(const ezDocumentObjectManager* pDocument);

  void SetSelection(const ezDocumentObject* pSingleObject)
  {
    ezDeque<const ezDocumentObject*> objs;
    objs.PushBack(pSingleObject);
    SetSelection(objs);
  }

  void Clear();
  void AddObject(const ezDocumentObject* pObject);
  void RemoveObject(const ezDocumentObject* pObject, bool bRecurseChildren = false);
  void SetSelection(const ezDeque<const ezDocumentObject*>& Selection);
  void ToggleObject(const ezDocumentObject* pObject);

  /// \brief Returns the last selected object in the selection or null if empty.
  const ezDocumentObject* GetCurrentObject() const;

  const ezDeque<const ezDocumentObject*>& GetSelection() const { return m_pSelectionStorage->m_SelectionList; }

  bool IsSelectionEmpty() const { return m_pSelectionStorage->m_SelectionList.IsEmpty(); }

  /// \brief Returns the subset of selected items which have no parent selected. Ie. if an object is selected and one of its ancestors is selected, it
  /// is culled from the list.
  const ezDeque<const ezDocumentObject*> GetTopLevelSelection() const;

  /// \brief Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase
  const ezDeque<const ezDocumentObject*> GetTopLevelSelection(const ezRTTI* pBase) const;

  bool IsSelected(const ezDocumentObject* pObject) const;
  bool IsParentSelected(const ezDocumentObject* pObject) const;

  const ezDocument* GetDocument() const;

  ezSharedPtr<ezSelectionManager::Storage> SwapStorage(ezSharedPtr<ezSelectionManager::Storage> pNewStorage);
  ezSharedPtr<ezSelectionManager::Storage> GetStorage() { return m_pSelectionStorage; }


private:
  void TreeEventHandler(const ezDocumentObjectStructureEvent& e);
  bool RecursiveRemoveFromSelection(const ezDocumentObject* pObject);

  friend class ezDocument;

  ezSharedPtr<ezSelectionManager::Storage> m_pSelectionStorage;

  ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&>::Unsubscriber m_EventsUnsubscriber;
  const ezDocumentObjectManager* m_pObjectManager = nullptr;
};
