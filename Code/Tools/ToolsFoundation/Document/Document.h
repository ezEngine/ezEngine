#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>
#include <CoreUtils/DataStructures/ObjectMetaData.h>
#include <Foundation/Types/UniquePtr.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectMetaData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentObjectMetaData, ezReflectedClass);

public:

  enum ModifiedFlags
  {
    HiddenFlag = EZ_BIT(0),
    PrefabFlag = EZ_BIT(1),

    AllFlags = 0xFFFFFFFF
  };

  ezDocumentObjectMetaData()
  {
    m_bHidden = false;
  }

  bool m_bHidden;
  ezUuid m_CreateFromPrefab;
  ezUuid m_PrefabSeedGuid;
  ezString m_sBasePrefab;
};

class EZ_TOOLSFOUNDATION_DLL ezDocument : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocument, ezReflectedClass);

public:
  ezDocument(const char* szPath, ezDocumentObjectManager* pDocumentObjectManagerImpl);
  virtual ~ezDocument();

  /// \name Document State Functions
  ///@{

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }
  const ezUuid& GetGuid() const { return m_pDocumentInfo->m_DocumentID; }
  virtual const char* GetDocumentTypeDisplayString() const = 0;

  const ezDocumentObjectManager* GetObjectManager() const { return m_pObjectManager; }
  ezDocumentObjectManager* GetObjectManager() { return m_pObjectManager; }
  ezSelectionManager* GetSelectionManager() const { return &m_SelectionManager; }
  ezCommandHistory* GetCommandHistory() const { return &m_CommandHistory; }

  virtual ezVariant GetDefaultValue(const ezDocumentObject* pObject, const ezPropertyPath& path) const;
  virtual bool IsDefaultValue(const ezDocumentObject* pObject, const ezPropertyPath& path) const;

  ///@}
  /// \name Document Management Functions
  ///@{

  /// \brief Returns the absolute path to the document.
  const char* GetDocumentPath() const { return m_sDocumentPath; }

  ezStatus SaveDocument();
  ezStatus LoadDocument() { return InternalLoadDocument(); }

  /// \brief Brings the corresponding window to the front.
  void EnsureVisible();

  void BroadcastSaveDocumentMetaState();

  ezDocumentManager* GetDocumentManager() const { return m_pDocumentManager; }

  bool HasWindowBeenRequested() const { return m_bWindowRequested; }

  const ezDocumentTypeDescriptor& GetDocumentTypeDescriptor() const { return m_TypeDescriptor; }

  const ezDocumentInfo* GetDocumentInfo() { return m_pDocumentInfo; }

  ///@}
  /// \name Clipboard Functions
  ///@{

  struct PasteInfo
  {
    EZ_DECLARE_POD_TYPE();

    ezDocumentObject* m_pObject;
    ezDocumentObject* m_pParent;
  };

  virtual bool Copy(ezAbstractObjectGraph& out_objectGraph) const { return false; };
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition) { return false; };

  ///@}
  /// \name Misc Functions
  ///@{

  virtual void DeleteSelectedObjects() const;

  const ezSet<ezString>& GetUnknownObjectTypes() const { return m_UnknownObjectTypes; }
  ezUInt32 GetUnknownObjectTypeInstances() const { return m_uiUnknownObjectTypeInstances; }

  /// \brief If disabled, this document will not be put into the recent files list.
  void SetAddToResetFilesList(bool b) { m_bAddToRecentFilesList = b; }

  /// \brief Whether this document shall be put into the recent files list.
  bool GetAddToRecentFilesList() const { return m_bAddToRecentFilesList; }

  /// \brief Broadcasts a status message event. The window that displays the document may show this in some form, e.g. in the status bar.
  void ShowDocumentStatus(const char* szFormat, ...) const;

  /// \brief Tries to compute the position and rotation for an object in the document. Returns EZ_SUCCESS if it was possible.
  virtual ezResult ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const { return EZ_FAILURE; }

  ///@}
  /// \name Prefab Functions
  ///@{

  virtual void UpdatePrefabs();
  virtual void RevertPrefabs(const ezDeque<const ezDocumentObject*>& Selection);
  virtual void UnlinkPrefabs(const ezDeque<const ezDocumentObject*>& Selection);

  virtual const ezString& GetCachedPrefabDocument(const ezUuid& documentGuid) const;
  virtual const ezAbstractObjectGraph* GetCachedPrefabGraph(const ezUuid& documentGuid) const;
  virtual ezStatus CreatePrefabDocumentFromSelection(const char* szFile, const ezRTTI* pRootType);
  virtual ezStatus CreatePrefabDocument(const char* szFile, const ezDocumentObject* pRootObject, const ezUuid& invPrefabSeed, ezUuid& out_NewDocumentGuid);
  // Returns new guid of replaced object.
  virtual ezUuid ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed);
  // Returns new guid of reverted object.
  virtual ezUuid RevertPrefab(const ezDocumentObject* pObject);

  ///@}

public:
  ezObjectMetaData<ezUuid, ezDocumentObjectMetaData> m_DocumentObjectMetaData;
  
  mutable ezEvent<const ezDocumentEvent&> m_EventsOne;
  static ezEvent<const ezDocumentEvent&> s_EventsAny;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  virtual ezStatus InternalSaveDocument();
  virtual ezStatus InternalLoadDocument();
  virtual ezDocumentInfo* CreateDocumentInfo() = 0;

  /// \brief A hook to execute additional code after SUCCESSFULLY saving a document. E.g. manual asset transform can be done here.
  virtual void InternalAfterSaveDocument() {}

  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph);

  virtual void InitializeBeforeLoading() {}
  virtual void InitializeAfterLoading() {}

  void SetUnknownObjectTypes(const ezSet<ezString>& Types, ezUInt32 uiInstances);

  /// \name Prefab Functions
  ///@{

  virtual void UpdatePrefabsRecursive(ezDocumentObject* pObject);
  virtual void UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab);
  ezString ReadDocumentAsString(const char* szFile) const;
  virtual ezString GetDocumentPathFromGuid(const ezUuid& documentGuid) const;

  ///@}

  mutable ezSelectionManager m_SelectionManager;
  mutable ezCommandHistory m_CommandHistory;
  ezDocumentInfo* m_pDocumentInfo;
  ezDocumentTypeDescriptor m_TypeDescriptor;

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

  mutable ezMap<ezUuid, ezString> m_CachedPrefabDocuments;
  mutable ezMap<ezUuid, ezUniquePtr<ezAbstractObjectGraph>> m_CachedPrefabGraphs;

};
