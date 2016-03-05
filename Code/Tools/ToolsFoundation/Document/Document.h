#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <Foundation/Communication/Event.h>

class ezDocument;
class ezDocumentManager;
class ezDocumentObjectManager;
class ezAbstractObjectGraph;

struct EZ_TOOLSFOUNDATION_DLL ezDocumentTypeDescriptor
{
  ezHybridArray<ezString, 4> m_sFileExtensions;
  ezString m_sDocumentTypeName;
  bool m_bCanCreate;
  ezString m_sIcon;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentInfo : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentInfo, ezReflectedClass);

public:
  ezDocumentInfo();

  ezUuid m_DocumentID;
};

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

  void SetAddToResetFilesList(bool b) { m_bAddToRecentFilesList = b; }
  bool GetAddToResetFilesList() const { return m_bAddToRecentFilesList; }

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
    const ezDocument* m_pDocument;
  };
  
  ezEvent<const Event&> m_EventsOne;
  static ezEvent<const Event&> s_EventsAny;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  virtual ezStatus InternalSaveDocument();
  virtual ezStatus InternalLoadDocument();
  virtual ezDocumentInfo* CreateDocumentInfo() = 0;

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
