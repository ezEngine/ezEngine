#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Basics/Guid.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <Foundation/Communication/Event.h>

struct EZ_TOOLSFOUNDATION_DLL ezDocumentInfo
{
  ezString m_sAbsoluteFilePath;
  ezGuid m_sDocumentGuid;
};

class ezDocumentBase;
class ezCommandHistoryBase;
class ezDocumentObjectManagerBase;

class EZ_TOOLSFOUNDATION_DLL ezDocumentBase
{
public:
  ezDocumentBase();
  virtual ~ezDocumentBase();

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }

  const ezCommandHistoryBase* GetCommandHistory() const { return m_pCommandHistory; }
  const ezDocumentObjectManagerBase* GetObjectManager() const { return m_pObjectManager; }
  const ezDocumentObjectTree* GetObjectTree() const { return &m_ObjectTree; }
  const ezSelectionManager* GetSelectionManager() const { return &m_SelectionManager; }

public:
  struct Event
  {
    enum class Type
    {
      ModifiedChanged,
      ReadOnlyChanged,
    };

    Type m_Type;

    const ezDocumentBase* m_pDocument;
  };

  ezEvent<const Event&> m_Events;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);

  ezCommandHistoryBase* m_pCommandHistory;
  ezDocumentObjectManagerBase* m_pObjectManager;
  ezDocumentObjectTree m_ObjectTree;
  ezSelectionManager m_SelectionManager;

private:
  bool m_bModified;
  bool m_bReadOnly;
};
