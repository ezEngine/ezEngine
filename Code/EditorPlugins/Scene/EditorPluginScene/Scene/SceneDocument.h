#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>

#include <EditorFramework/Document/GameObjectDocument.h>

class ezExposedSceneProperty;
class ezSceneDocumentSettingsBase;
class ezPushObjectStateMsgToEditor;

struct GameMode
{
  enum Enum
  {
    Off,
    Simulate,
    Play,
  };
};

class EZ_EDITORPLUGINSCENE_DLL ezSceneDocument : public ezGameObjectDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocument, ezGameObjectDocument);

public:
  enum class DocumentType
  {
    Scene,
    Prefab,
    Layer
  };

public:
  ezSceneDocument(ezStringView sDocumentPath, DocumentType documentType);
  ~ezSceneDocument();

  enum class ShowOrHide
  {
    Show,
    Hide
  };

  /// \brief Creates a new object and attaches all currently selected objects to it.
  void GroupSelection();

  /// \brief Changes the selection to the parent object.
  void SelectParentObject();

  /// \brief Sets the last selected object as the 'active parent'.
  void SetSelectedAsActiveParent();
  /// \brief Clears the 'active parent' object.
  void ClearActiveParent();

  /// \brief Opens the Duplicate Special dialog
  void DuplicateSpecial();

  /// \brief Opens the 'Delta Transform' dialog.
  void DeltaTransform();


  /// \brief Moves all selected objects to the editor camera position
  void SnapObjectToCamera();


  /// \brief Attaches all selected objects to the selected object
  void AttachToObject();

  /// \brief Detaches all selected objects from their current parent
  void DetachFromParent();

  /// \brief Puts the GUID of the single selected object into the clipboard
  void CopyReference();

  /// \brief Creates a new empty object, either top-level (selection empty) or as a child of the selected item
  ezStatus CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition, bool bComponentSelectionMenu);

  void DuplicateSelection();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();

  /// \brief Whether this document represents a prefab or a scene
  bool IsPrefab() const { return m_DocumentType == DocumentType::Prefab; }

  /// \brief Determines whether the given object is an editor prefab
  bool IsObjectEditorPrefab(const ezUuid& object, ezUuid* out_pPrefabAssetGuid = nullptr) const;

  /// \brief Determines whether the given object is an engine prefab
  bool IsObjectEnginePrefab(const ezUuid& object, ezUuid* out_pPrefabAssetGuid = nullptr) const;

  /// \brief Nested prefabs are not allowed
  virtual bool ArePrefabsAllowed() const override { return !IsPrefab(); }


  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_sMimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType) override;
  bool DuplicateSelectedObjects(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bSetSelected);
  bool CopySelectedObjects(ezAbstractObjectGraph& ref_graph, ezMap<ezUuid, ezUuid>* out_pParents) const;
  bool PasteAt(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, const ezVec3& vPos);
  bool PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph);

  virtual void UpdatePrefabs() override;

  /// \brief Removes the link to the prefab template, making the editor prefab a simple object
  virtual void UnlinkPrefabs(ezArrayPtr<const ezDocumentObject*> selection) override;

  virtual ezUuid ReplaceByPrefab(const ezDocumentObject* pRootObject, ezStringView sPrefabFile, const ezUuid& prefabAsset, const ezUuid& prefabSeed, bool bEnginePrefab) override;

  /// \brief Reverts all selected editor prefabs to their original template state
  virtual ezUuid RevertPrefab(const ezDocumentObject* pObject) override;

  /// \brief Converts all objects in the selection that are engine prefabs to their respective editor prefab representation
  virtual void ConvertToEditorPrefab(ezArrayPtr<const ezDocumentObject*> selection);
  /// \brief Converts all objects in the selection that are editor prefabs to their respective engine prefab representation
  virtual void ConvertToEnginePrefab(ezArrayPtr<const ezDocumentObject*> selection);

  virtual ezStatus CreatePrefabDocumentFromSelection(ezStringView sFile, const ezRTTI* pRootType, ezDelegate<void(ezAbstractObjectNode*)> adjustGraphNodeCB = {}, ezDelegate<void(ezDocumentObject*)> adjustNewNodesCB = {}, ezDelegate<void(ezAbstractObjectGraph& graph, ezDynamicArray<ezAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB = {}) override;

  GameMode::Enum GetGameMode() const { return m_GameMode; }

  virtual bool CanEngineProcessBeRestarted() const override;

  void StartSimulateWorld();
  void TriggerGameModePlay(bool bUsePickedPositionAsStart);

  /// Stops the world simulation, if it is running. Returns true, when the simulation needed to be stopped.
  bool StopGameMode();

  void StepSimulation();
  void PauseSimulation();

  ezTransformStatus ExportScene(bool bCreateThumbnail);
  void ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode /* ezWorldGeoExtractionUtil::ExtractionMode */, const ezMat3& mTransform);

  virtual void HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  void HandleGameModeMsg(const ezGameModeMsgToEditor* pMsg);
  void HandleObjectStateFromEngineMsg(const ezPushObjectStateMsgToEditor* pMsg);

  void SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);

  /// \name Scene Settings
  ///@{

  virtual const ezDocumentObject* GetSettingsObject() const;
  const ezSceneDocumentSettingsBase* GetSettingsBase() const;
  template <typename T>
  const T* GetSettings() const
  {
    return ezDynamicCast<const T*>(GetSettingsBase());
  }

  ezStatus CreateExposedProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index, ezExposedSceneProperty& out_key) const;
  ezStatus AddExposedParameter(const char* szName, const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index);
  ezInt32 FindExposedParameter(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index);
  ezStatus RemoveExposedParameter(ezInt32 iIndex);
  ///@}

  /// \name Editor Camera
  ///@{

  /// \brief Stores the current editor camera position in a user preference. Slot can be 0 to 9.
  ///
  /// Since the preference is stored on disk, this position can be restored in another session.
  void StoreFavoriteCamera(ezUInt8 uiSlot);

  /// \brief Applies the previously stored camera position from slot 0 to 9 to the current camera position.
  ///
  /// The camera will quickly interpolate to the stored position.
  void RestoreFavoriteCamera(ezUInt8 uiSlot);

  /// \brief Searches for an ezCameraComponent with the 'EditorShortcut' property set to \a uiSlot and moves the editor camera to that position.
  ezResult JumpToLevelCamera(ezUInt8 uiSlot, bool bImmediate);

  /// \brief Creates an object with an ezCameraComponent at the current editor camera position and sets the 'EditorShortcut' property to \a uiSlot.
  ezResult CreateLevelCamera(ezUInt8 uiSlot);

  virtual ezManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return ezManipulatorSearchStrategy::ChildrenOfSelectedObject;
  }

  ///@}

  bool CanUndoSelection() const;
  virtual void UndoSelection();

protected:
  void SetGameMode(GameMode::Enum mode);

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, ezStringView sBasePrefab) override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;

  template <typename Func>
  void ApplyRecursive(const ezDocumentObject* pObject, Func f)
  {
    f(pObject);

    for (auto pChild : pObject->GetChildren())
    {
      ApplyRecursive<Func>(pChild, f);
    }
  }

protected:
  void EnsureSettingsObjectExist();
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void EngineConnectionEventHandler(const ezEditorEngineProcessConnection::Event& e);
  void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

  ezStatus RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header);

  virtual ezTransformStatus InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  virtual ezTransformStatus InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
    const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags) override;
  ezTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(ezDocumentObject* pObject);

  /// \brief Finds all objects that are actively being 'debugged' (or visualized) by the editor and thus should get the debug visualization flag in
  /// the runtime.
  void UpdateObjectDebugTargets();

  DocumentType m_DocumentType = DocumentType::Scene;

  GameMode::Enum m_GameMode;

  GameModeData m_GameModeData[3];

  // Local mirror for settings
  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;

  //////////////////////////////////////////////////////////////////////////
protected:
  bool m_bStoreSelectionChange = true;
  ezInt8 m_iAllowSelectionChanges = -1;
  ezCopyOnBroadcastEvent<const ezSelectionManagerEvent&>::Unsubscriber m_SelectionHandlerUnsubscriber;
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);

  struct SelectionHistory
  {
    ezDynamicArray<ezUuid> m_Objects;
    ezUuid m_documentGuid;
  };

  ezDeque<SelectionHistory> m_SelectionStack;

  //////////////////////////////////////////////////////////////////////////
  /// Communication with other document types
  virtual void OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender) override;
  void GatherObjectsOfType(ezDocumentObject* pRoot, ezGatherObjectsOfTypeMsgInterDoc* pMsg) const;
};
