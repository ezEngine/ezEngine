#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>

class EZ_TOOLSFOUNDATION_DLL ezDocument : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocument, ezReflectedClass);

public:
  ezDocument(const char* szPath, ezDocumentObjectManager* pDocumentObjectManagerImpl);
  virtual ~ezDocument();

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }
  const ezUuid& GetGuid() const { return m_pDocumentInfo->m_DocumentID; }
  virtual const char* GetDocumentTypeDisplayString() const = 0;

  const ezDocumentObjectManager* GetObjectManager() const { return m_pObjectManager; }
  const ezSelectionManager* GetSelectionManager() const { return &m_SelectionManager; }
  ezCommandHistory* GetCommandHistory() const { return &m_CommandHistory; }

  ezDocumentObjectManager* GetObjectManager() { return m_pObjectManager; }
  ezSelectionManager* GetSelectionManager() { return &m_SelectionManager; }

  const char* GetDocumentPath() const { return m_sDocumentPath; }

  ezStatus SaveDocument();
  ezStatus LoadDocument() { return InternalLoadDocument(); }
  void EnsureVisible();

  void BroadcastSaveDocumentMetaState();

  ezDocumentManager* GetDocumentManager() const { return m_pDocumentManager; }

  bool HasWindowBeenRequested() const { return m_bWindowRequested; }

  const ezDocumentTypeDescriptor& GetDocumentTypeDescriptor() const { return m_TypeDescriptor; }

  const ezDocumentInfo* GetDocumentInfo() { return m_pDocumentInfo; }

  struct PasteInfo
  {
    EZ_DECLARE_POD_TYPE();

    ezDocumentObject* m_pObject;
    ezDocumentObject* m_pParent;
  };

  virtual bool Copy(ezAbstractObjectGraph& out_objectGraph) { return false; };
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition) { return false; };
  virtual void DeleteSelectedObjects();

  const ezSet<ezString>& GetUnknownObjectTypes() const { return m_UnknownObjectTypes; }
  ezUInt32 GetUnknownObjectTypeInstances() const { return m_uiUnknownObjectTypeInstances; }

  /// \brief If disabled, this document will not be put into the recent files list.
  void SetAddToResetFilesList(bool b) { m_bAddToRecentFilesList = b; }

  /// \brief Whether this document shall be put into the recent files list.
  bool GetAddToRecentFilesList() const { return m_bAddToRecentFilesList; }

  /// \brief Broadcasts a status message event. The window that displays the document may show this in some form, e.g. in the status bar.
  void ShowDocumentStatus(const char* szFormat, ...);

public:
  
  ezEvent<const ezDocumentEvent&> m_EventsOne;
  static ezEvent<const ezDocumentEvent&> s_EventsAny;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  virtual ezStatus InternalSaveDocument();
  virtual ezStatus InternalLoadDocument();
  virtual ezDocumentInfo* CreateDocumentInfo() = 0;

  /// \brief A hook to execute additional code after SUCCESSFULLY saving a document. E.g. manual asset transform can be done here.
  virtual void InternalAfterSaveDocument() {}

  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) {}
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph) {}

  virtual void InitializeBeforeLoading() { }
  virtual void InitializeAfterLoading() { }

  ezSelectionManager m_SelectionManager;
  mutable ezCommandHistory m_CommandHistory;
  ezDocumentInfo* m_pDocumentInfo;
  ezDocumentTypeDescriptor m_TypeDescriptor;

  void SetUnknownObjectTypes(const ezSet<ezString>& Types, ezUInt32 uiInstances);

private:
  friend class ezDocumentManager;
  friend class ezCommandHistory;

  void SetupDocumentInfo(const ezDocumentTypeDescriptor& TypeDescriptor);

  ezDocumentObjectManager* m_pObjectTree;
  ezDocumentManager* m_pDocumentManager;
  ezDocumentObjectManager* m_pObjectManager;

  ezString m_sDocumentPath;
  bool m_bModified;
  bool m_bReadOnly;
  bool m_bWindowRequested;
  bool m_bAddToRecentFilesList;

  ezSet<ezString> m_UnknownObjectTypes;
  ezUInt32 m_uiUnknownObjectTypeInstances;
};
