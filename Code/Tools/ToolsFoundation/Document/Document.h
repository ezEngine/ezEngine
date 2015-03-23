#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Basics/Guid.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Object/DocumentObjectTree.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <Foundation/Communication/Event.h>

class ezDocumentBase;
class ezDocumentManagerBase;
class ezCommandHistoryBase;
class ezDocumentObjectManagerBase;

class EZ_TOOLSFOUNDATION_DLL ezDocumentInfo : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentInfo);

public:
  ezDocumentInfo();

  ezUuid m_DocumentID;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentBase);

public:
  ezDocumentBase(const char* szPath, ezDocumentObjectManagerBase* pDocumentObjectManagerImpl);
  virtual ~ezDocumentBase();

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }
  const ezUuid& GetGuid() const { return m_documentInfo.m_DocumentID; }
  virtual const char* GetDocumentTypeDisplayString() const = 0;

  const ezDocumentObjectManagerBase* GetObjectManager() const { return m_pObjectManager; }
  const ezDocumentObjectTree* GetObjectTree() const { return m_pObjectTree; }
  const ezSelectionManager* GetSelectionManager() const { return &m_SelectionManager; }
  ezCommandHistory* GetCommandHistory() const { return &m_CommandHistory; }

  ezDocumentObjectManagerBase* GetObjectManager() { return m_pObjectManager; }
  ezDocumentObjectTree* GetObjectTree() { return m_pObjectTree; }
  ezSelectionManager* GetSelectionManager() { return &m_SelectionManager; }

  const char* GetDocumentPath() const { return m_sDocumentPath; }

  ezStatus SaveDocument();
  ezStatus LoadDocument() { return InternalLoadDocument(); }
  void EnsureVisible();

  void BroadcastSaveDocumentMetaState();

  ezDocumentManagerBase* GetDocumentManager() const { return m_pDocumentManager; }

public:
  struct Event
  {
    enum class Type
    {
      ModifiedChanged,
      ReadOnlyChanged,
      EnsureVisible,
      DocumentSaved,
      SaveDocumentMetaState,
    };

    Type m_Type;
    const ezDocumentBase* m_pDocument;
  };
  
  ezEvent<const Event&> m_EventsOne;
  static ezEvent<const Event&> s_EventsAny;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  virtual ezStatus InternalSaveDocument();
  virtual ezStatus InternalLoadDocument();

  ezSelectionManager m_SelectionManager;
  mutable ezCommandHistory m_CommandHistory;

private:
  friend class ezDocumentManagerBase;
  friend class ezCommandHistory;

  ezDocumentObjectTree* m_pObjectTree;
  ezDocumentManagerBase* m_pDocumentManager;
  ezDocumentObjectManagerBase* m_pObjectManager;

  ezString m_sDocumentPath;
  bool m_bModified;
  bool m_bReadOnly;
  ezDocumentInfo m_documentInfo;
};
