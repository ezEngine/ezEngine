#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Basics/Guid.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <Foundation/Communication/Event.h>

class ezDocumentBase;
class ezDocumentManagerBase;
class ezCommandHistoryBase;
class ezDocumentObjectManagerBase;

class EZ_TOOLSFOUNDATION_DLL ezDocumentBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentBase);

public:
  ezDocumentBase(const char* szPath);
  virtual ~ezDocumentBase();

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }
  virtual const char* GetDocumentTypeDisplayString() const = 0;

  const ezCommandHistoryBase* GetCommandHistory() const { return m_pCommandHistory; }
  const ezDocumentObjectManagerBase* GetObjectManager() const { return m_pObjectManager; }
  const ezDocumentObjectTree* GetObjectTree() const { return &m_ObjectTree; }
  const ezSelectionManager* GetSelectionManager() const { return &m_SelectionManager; }

  const char* GetDocumentPath() const { return m_sDocumentPath; }

  ezStatus SaveDocument();
  ezStatus LoadDocument() { return InternalLoadDocument(); }
  void EnsureVisible();

  ezDocumentManagerBase* GetDocumentManager() const { return m_pDocumentManager; }

public:
  struct Event
  {
    enum class Type
    {
      ModifiedChanged,
      ReadOnlyChanged,
      EnsureVisible,
    };

    Type m_Type;

    const ezDocumentBase* m_pDocument;
  };

  ezEvent<const Event&> m_Events;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  virtual ezStatus InternalSaveDocument() = 0;
  virtual ezStatus InternalLoadDocument() = 0;

  ezCommandHistoryBase* m_pCommandHistory;
  ezDocumentObjectManagerBase* m_pObjectManager;
  ezDocumentObjectTree m_ObjectTree;
  ezSelectionManager m_SelectionManager;

private:
  friend class ezDocumentManagerBase;
  ezDocumentManagerBase* m_pDocumentManager;

  ezString m_sDocumentPath;
  bool m_bModified;
  bool m_bReadOnly;
};
