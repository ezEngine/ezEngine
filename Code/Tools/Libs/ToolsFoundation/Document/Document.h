#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Implementation/Declarations.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezObjectAccessorBase;
class ezObjectCommandAccessor;
class ezEditorInputContext;
class ezAbstractObjectNode;

struct EZ_TOOLSFOUNDATION_DLL ezObjectAccessorChangeEvent
{
  ezDocument* m_pDocument;
  ezObjectAccessorBase* m_pOldObjectAccessor;
  ezObjectAccessorBase* m_pNewObjectAccessor;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectMetaData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentObjectMetaData, ezReflectedClass);

public:
  enum ModifiedFlags : unsigned int
  {
    HiddenFlag = EZ_BIT(0),
    PrefabFlag = EZ_BIT(1),
    ActiveParentFlag = EZ_BIT(2), /// This flag is used to update an entry, even though there is no meta data for it.

    AllFlags = 0xFFFFFFFF
  };

  bool m_bHidden = false;    /// Whether the object should be rendered in the editor view (no effect on the runtime)
  ezUuid m_CreateFromPrefab; /// The asset GUID of the prefab from which this object was created. Invalid GUID, if this is not a prefab instance.
  ezUuid m_PrefabSeedGuid;   /// The seed GUID used to remap the object GUIDs from the prefab asset into this instance.
  ezString m_sBasePrefab;    /// The prefab from which this instance was created as complete DDL text (this describes the entire object!). Necessary for
                             /// three-way-merging the prefab instances.
};

enum class ezManipulatorSearchStrategy
{
  None,
  SelectedObject,
  ChildrenOfSelectedObject
};

class EZ_TOOLSFOUNDATION_DLL ezDocument : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocument, ezReflectedClass);

public:
  ezDocument(ezStringView sPath, ezDocumentObjectManager* pDocumentObjectManagerImpl);
  virtual ~ezDocument();

  /// \name Document State Functions
  ///@{

  bool IsModified() const { return m_bModified; }
  bool IsReadOnly() const { return m_bReadOnly; }
  const ezUuid GetGuid() const { return m_pDocumentInfo ? m_pDocumentInfo->m_DocumentID : ezUuid(); }

  const ezDocumentObjectManager* GetObjectManager() const { return m_pObjectManager.Borrow(); }
  ezDocumentObjectManager* GetObjectManager() { return m_pObjectManager.Borrow(); }
  ezSelectionManager* GetSelectionManager() const { return m_pSelectionManager.Borrow(); }
  ezCommandHistory* GetCommandHistory() const { return m_pCommandHistory.Borrow(); }
  virtual ezObjectAccessorBase* GetObjectAccessor() const;

  ///@}
  /// \name Main / Sub-Document Functions
  ///@{

  /// \brief Returns whether this document is a main document, i.e. self contained.
  bool IsMainDocument() const { return m_pHostDocument == this; }
  /// \brief Returns whether this document is a sub-document, i.e. is part of another document.
  bool IsSubDocument() const { return m_pHostDocument != this; }
  /// \brief In case this is a sub-document, returns the main document this belongs to. Otherwise 'this' is returned.
  const ezDocument* GetMainDocument() const { return m_pHostDocument; }
  /// \brief At any given time, only the active sub-document can be edited. This returns the active sub-document which can also be this document itself. Changes to the active sub-document are generally triggered by ezDocumentObjectStructureEvent::Type::AfterReset.
  const ezDocument* GetActiveSubDocument() const { return m_pActiveSubDocument; }
  ezDocument* GetMainDocument() { return m_pHostDocument; }
  ezDocument* GetActiveSubDocument() { return m_pActiveSubDocument; }

protected:
  ezDocument* m_pHostDocument = nullptr;
  ezDocument* m_pActiveSubDocument = nullptr;

  ///@}
  /// \name Document Management Functions
  ///@{

public:
  /// \brief Returns the absolute path to the document.
  ezStringView GetDocumentPath() const { return m_sDocumentPath; }

  /// \brief Saves the document, if it is modified.
  /// If bForce is true, the document will be written, even if it is not considered modified.
  ezStatus SaveDocument(bool bForce = false);
  using AfterSaveCallback = ezDelegate<void(ezDocument*, ezStatus)>;
  ezTaskGroupID SaveDocumentAsync(AfterSaveCallback callback, bool bForce = false);
  void DocumentRenamed(ezStringView sNewDocumentPath);

  static ezStatus ReadDocument(ezStringView sDocumentPath, ezUniquePtr<ezAbstractObjectGraph>& ref_pHeader, ezUniquePtr<ezAbstractObjectGraph>& ref_pObjects,
    ezUniquePtr<ezAbstractObjectGraph>& ref_pTypes);
  static ezStatus ReadAndRegisterTypes(const ezAbstractObjectGraph& types);

  ezStatus LoadDocument() { return InternalLoadDocument(); }

  /// \brief Brings the corresponding window to the front.
  void EnsureVisible();

  ezDocumentManager* GetDocumentManager() const { return m_pDocumentManager; }

  bool HasWindowBeenRequested() const { return m_bWindowRequested; }

  const ezDocumentTypeDescriptor* GetDocumentTypeDescriptor() const { return m_pTypeDescriptor; }

  /// \brief Returns the document's type name. Same as GetDocumentTypeDescriptor()->m_sDocumentTypeName.
  ezStringView GetDocumentTypeName() const
  {
    if (m_pTypeDescriptor == nullptr)
    {
      // if this is a document without a type descriptor, use the RTTI type name as a fallback
      return GetDynamicRTTI()->GetTypeName();
    }

    return m_pTypeDescriptor->m_sDocumentTypeName;
  }

  const ezDocumentInfo* GetDocumentInfo() const { return m_pDocumentInfo; }

  /// \brief Asks the document whether a restart of the engine process is allowed at this time.
  ///
  /// Documents that are currently interacting with the engine process (active play-the-game mode) should return false.
  /// All others should return true.
  /// As long as any document returns false, automatic engine process reload is suppressed.
  virtual bool CanEngineProcessBeRestarted() const { return true; }

  ///@}
  /// \name Clipboard Functions
  ///@{

  struct PasteInfo
  {
    EZ_DECLARE_POD_TYPE();

    ezDocumentObject* m_pObject = nullptr;
    ezDocumentObject* m_pParent = nullptr;
    ezInt32 m_Index = -1;
  };

  /// \brief Whether this document supports pasting the given mime format into it
  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_mimeTypes) const {}
  /// \brief Creates the abstract graph of data to be copied and returns the mime type for the clipboard to identify the data
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_sMimeType) const { return false; };
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType)
  {
    return false;
  };

  ///@}
  /// \name Inter Document Communication
  ///@{

  /// \brief This will deliver the message to all open documents. The documents may respond, e.g. by modifying the content of the message.
  void BroadcastInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender);

  /// \brief Called on all documents when BroadcastInterDocumentMessage() is called.
  ///
  /// Use the RTTI information to identify whether the message is of interest.
  virtual void OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender) {}

  ///@}
  /// \name Editing Functionality
  ///@{

  /// \brief Allows to return a single input context that currently overrides all others (in priority).
  ///
  /// Used to implement custom tools that need to have priority over selection and camera movement.
  virtual ezEditorInputContext* GetEditorInputContextOverride() { return nullptr; }

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
  void ShowDocumentStatus(const ezFormatString& msg) const;

  /// \brief Tries to compute the position and rotation for an object in the document. Returns EZ_SUCCESS if it was possible.
  virtual ezResult ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_result) const;

  /// \brief Needed by ezManipulatorManager to know where to look for the manipulator attributes.
  ///
  /// Override this function for document types that use manipulators.
  /// The ezManipulatorManager will assert that the document type doesn't return 'None' once it is in use.
  virtual ezManipulatorSearchStrategy GetManipulatorSearchStrategy() const { return ezManipulatorSearchStrategy::None; }

  ///@}
  /// \name Prefab Functions
  ///@{

  /// \brief Whether the document allows to create prefabs in it. This may note be allowed for prefab documents themselves, to prevent nested prefabs.
  virtual bool ArePrefabsAllowed() const { return true; }

  /// \brief Updates ALL prefabs in the document with the latest changes. Merges the current prefab templates with the instances in the document.
  virtual void UpdatePrefabs();

  /// \brief Resets the given objects to their template prefab state, if they have local modifications.
  void RevertPrefabs(ezArrayPtr<const ezDocumentObject*> selection);

  /// \brief Removes the link between a prefab instance and its template, turning the instance into a regular object.
  virtual void UnlinkPrefabs(ezArrayPtr<const ezDocumentObject*> selection);

  virtual ezStatus CreatePrefabDocumentFromSelection(ezStringView sFile, const ezRTTI* pRootType, ezDelegate<void(ezAbstractObjectNode*)> adjustGraphNodeCB = {}, ezDelegate<void(ezDocumentObject*)> adjustNewNodesCB = {}, ezDelegate<void(ezAbstractObjectGraph& graph, ezDynamicArray<ezAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});
  virtual ezStatus CreatePrefabDocument(ezStringView sFile, ezArrayPtr<const ezDocumentObject*> rootObjects, const ezUuid& invPrefabSeed, ezUuid& out_newDocumentGuid, ezDelegate<void(ezAbstractObjectNode*)> adjustGraphNodeCB = {}, bool bKeepOpen = false, ezDelegate<void(ezAbstractObjectGraph& graph, ezDynamicArray<ezAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {});

  // Returns new guid of replaced object.
  virtual ezUuid ReplaceByPrefab(const ezDocumentObject* pRootObject, ezStringView sPrefabFile, const ezUuid& prefabAsset, const ezUuid& prefabSeed, bool bEnginePrefab);
  // Returns new guid of reverted object.
  virtual ezUuid RevertPrefab(const ezDocumentObject* pObject);

  ///@}

public:
  ezUniquePtr<ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>> m_DocumentObjectMetaData;

  mutable ezEvent<const ezDocumentEvent&> m_EventsOne;
  static ezEvent<const ezDocumentEvent&> s_EventsAny;

  mutable ezEvent<const ezObjectAccessorChangeEvent&> m_ObjectAccessorChangeEvents;

protected:
  void SetModified(bool b);
  void SetReadOnly(bool b);
  virtual ezTaskGroupID InternalSaveDocument(AfterSaveCallback callback);
  virtual ezStatus InternalLoadDocument();
  virtual ezDocumentInfo* CreateDocumentInfo() = 0;

  /// \brief A hook to execute additional code after SUCCESSFULLY saving a document. E.g. manual asset transform can be done here.
  virtual void InternalAfterSaveDocument() {}

  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) {}
  virtual void InitializeAfterLoadingAndSaving() {}

  virtual void BeforeClosing();

  void SetUnknownObjectTypes(const ezSet<ezString>& Types, ezUInt32 uiInstances);

  /// \name Prefab Functions
  ///@{

  virtual void UpdatePrefabsRecursive(ezDocumentObject* pObject);
  virtual void UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, ezStringView sBasePrefab);

  ///@}

  ezUniquePtr<ezDocumentObjectManager> m_pObjectManager;
  mutable ezUniquePtr<ezCommandHistory> m_pCommandHistory;
  mutable ezUniquePtr<ezSelectionManager> m_pSelectionManager;
  mutable ezUniquePtr<ezObjectCommandAccessor> m_pObjectAccessor; ///< Default object accessor used by every doc.

  ezDocumentInfo* m_pDocumentInfo = nullptr;
  const ezDocumentTypeDescriptor* m_pTypeDescriptor = nullptr;

private:
  friend class ezDocumentManager;
  friend class ezCommandHistory;
  friend class ezSaveDocumentTask;
  friend class ezAfterSaveDocumentTask;

  void SetupDocumentInfo(const ezDocumentTypeDescriptor* pTypeDescriptor);

  ezDocumentManager* m_pDocumentManager = nullptr;

  ezString m_sDocumentPath;
  bool m_bModified;
  bool m_bReadOnly;
  bool m_bWindowRequested;
  bool m_bAddToRecentFilesList;

  ezSet<ezString> m_UnknownObjectTypes;
  ezUInt32 m_uiUnknownObjectTypeInstances;

  ezTaskGroupID m_ActiveSaveTask;
  ezStatus m_LastSaveResult = EZ_SUCCESS;
};
