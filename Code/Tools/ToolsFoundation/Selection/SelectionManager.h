#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocumentBase;
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
    const ezDocumentObjectBase* m_pObject;
  };

  ezEvent<const Event&> m_Events;

public:
  ezSelectionManager();


  void SetSelection(const ezDocumentObjectBase* pSingleObject)
  {
    Clear();
    AddObject(pSingleObject);
  }

  void Clear();
  void AddObject(const ezDocumentObjectBase* pObject);
  void RemoveObject(const ezDocumentObjectBase* pObject, bool bRecurseChildren = false);
  void SetSelection(const ezDeque<const ezDocumentObjectBase*>& Selection);
  void ToggleObject(const ezDocumentObjectBase* pObject);

  /// \brief Returns the last selected object in the selection or null if empty.
  const ezDocumentObjectBase* GetCurrentObject() const;

  const ezDeque<const ezDocumentObjectBase*>& GetSelection() const { return m_SelectionList; }

  bool IsSelectionEmpty() const { return m_SelectionList.IsEmpty(); }

  /// \brief Returns the subset of selected items which have no parent selected. Ie. if an object is selected and one of its ancestors is selected, it is culled from the list.
  const ezDeque<const ezDocumentObjectBase*> GetTopLevelSelection() const;

  /// \brief Same as GetTopLevelSelection() but additionally requires that all objects are derived from type pBase
  const ezDeque<const ezDocumentObjectBase*> GetTopLevelSelection(const ezRTTI* pBase) const;

  bool IsSelected(const ezDocumentObjectBase* pObject) const;
  bool IsParentSelected(const ezDocumentObjectBase* pObject) const;

  const ezDocumentBase* GetDocument() const { return m_pDocument; }


private:
  void TreeEventHandler(const ezDocumentObjectStructureEvent& e);
  bool RecursiveRemoveFromSelection(const ezDocumentObjectBase* pObject);

  friend class ezDocumentBase;

  void SetOwner(const ezDocumentBase* pDocument);

  ezDeque<const ezDocumentObjectBase*> m_SelectionList;
  ezSet<ezUuid> m_SelectionSet;
  const ezDocumentBase* m_pDocument;
};
