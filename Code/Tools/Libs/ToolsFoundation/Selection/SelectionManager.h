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

/// \brief Selection Manager stores a set of selected document objects.
class EZ_TOOLSFOUNDATION_DLL ezSelectionManager
{
public:
  ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&> m_Events;

  // \brief Storage for the selection so it can be swapped when using multiple sub documents.
  class Storage : public ezRefCounted
  {
  public:
    ezDeque<const ezDocumentObject*> m_SelectionList;
    ezSet<ezUuid> m_SelectionSet;
    const ezDocumentObjectManager* m_pObjectManager = nullptr;
    ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&> m_Events;
  };

public:
  ezSelectionManager(const ezDocumentObjectManager* pObjectManager);
  ~ezSelectionManager();

  void Clear();
  void AddObject(const ezDocumentObject* pObject);
  void RemoveObject(const ezDocumentObject* pObject, bool bRecurseChildren = false);
  void SetSelection(const ezDocumentObject* pSingleObject);
  void SetSelection(const ezDeque<const ezDocumentObject*>& selection);
  void ToggleObject(const ezDocumentObject* pObject);

  /// \brief Returns the last selected object in the selection or null if empty.
  const ezDocumentObject* GetCurrentObject() const;

  /// \brief Returns the selection in the same order the objects were added to the list.
  const ezDeque<const ezDocumentObject*>& GetSelection() const { return m_pSelectionStorage->m_SelectionList; }

  bool IsSelectionEmpty() const { return m_pSelectionStorage->m_SelectionList.IsEmpty(); }

  /// \brief Returns the subset of selected items which have no parent selected.
  ///
  /// I.e. if an object is selected and one of its ancestors is selected, it is culled from the list.
  /// if bInOrderOfSceneTree is true, items are returned in the order of appearance in an expanded scene tree.
  /// Otherwise they are returned in the order that they were selected.
  const ezDeque<const ezDocumentObject*> GetTopLevelSelection(bool bInOrderOfSceneTree) const;

  /// \brief Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase.
  const ezDeque<const ezDocumentObject*> GetTopLevelSelection(const ezRTTI* pBase, bool bInOrderOfSceneTree) const;

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

  ezCopyOnBroadcastEvent<const ezDocumentObjectStructureEvent&>::Unsubscriber m_ObjectStructureUnsubscriber;
  ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&>::Unsubscriber m_EventsUnsubscriber;
};
