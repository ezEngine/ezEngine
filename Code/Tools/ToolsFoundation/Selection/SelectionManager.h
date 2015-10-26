#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocument;
struct ezDocumentObjectStructureEvent;

class EZ_TOOLSFOUNDATION_DLL ezSelectionManager
{
public:

  struct Event
  {
    enum class Type
    {
      SelectionCleared,
      SelectionSet,
      ObjectAdded,
      ObjectRemoved,
    };

    Type m_Type;
    const ezDocumentObject* m_pObject;
  };

  ezEvent<const Event&> m_Events;

public:
  ezSelectionManager();


  void SetSelection(const ezDocumentObject* pSingleObject)
  {
    Clear();
    AddObject(pSingleObject);
  }

  void Clear();
  void AddObject(const ezDocumentObject* pObject);
  void RemoveObject(const ezDocumentObject* pObject, bool bRecurseChildren = false);
  void SetSelection(const ezDeque<const ezDocumentObject*>& Selection);
  void ToggleObject(const ezDocumentObject* pObject);

  /// \brief Returns the last selected object in the selection or null if empty.
  const ezDocumentObject* GetCurrentObject() const;

  const ezDeque<const ezDocumentObject*>& GetSelection() const { return m_SelectionList; }

  bool IsSelectionEmpty() const { return m_SelectionList.IsEmpty(); }

  /// \brief Returns the subset of selected items which have no parent selected. Ie. if an object is selected and one of its ancestors is selected, it is culled from the list.
  const ezDeque<const ezDocumentObject*> GetTopLevelSelection() const;

  /// \brief Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase
  const ezDeque<const ezDocumentObject*> GetTopLevelSelection(const ezRTTI* pBase) const;

  bool IsSelected(const ezDocumentObject* pObject) const;
  bool IsParentSelected(const ezDocumentObject* pObject) const;

  const ezDocument* GetDocument() const { return m_pDocument; }


private:
  void TreeEventHandler(const ezDocumentObjectStructureEvent& e);
  bool RecursiveRemoveFromSelection(const ezDocumentObject* pObject);

  friend class ezDocument;

  void SetOwner(const ezDocument* pDocument);

  ezDeque<const ezDocumentObject*> m_SelectionList;
  ezSet<ezUuid> m_SelectionSet;
  const ezDocument* m_pDocument;
};
