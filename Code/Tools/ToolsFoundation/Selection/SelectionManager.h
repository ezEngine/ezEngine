#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocumentBase;
struct ezDocumentObjectTreeStructureEvent;

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
  void RemoveObject(const ezDocumentObjectBase* pObject);
  void SetSelection(const ezDeque<const ezDocumentObjectBase*>& Selection);
  void ToggleObject(const ezDocumentObjectBase* pObject);

  const ezDeque<const ezDocumentObjectBase*>& GetSelection() const { return m_SelectionList; }

  bool IsSelected(const ezDocumentObjectBase* pObject) const;

  const ezDocumentBase* GetDocument() const { return m_pDocument; }

private:
  void TreeEventHandler(const ezDocumentObjectTreeStructureEvent& e);

  friend class ezDocumentBase;

  void SetOwner(const ezDocumentBase* pDocument);

  ezDeque<const ezDocumentObjectBase*> m_SelectionList;
  ezSet<ezUuid> m_SelectionSet;
  const ezDocumentBase* m_pDocument;
};
