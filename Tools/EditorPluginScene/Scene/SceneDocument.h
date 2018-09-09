#pragma once

#include <EditorFramework/Document/GameObjectDocument.h>

class ezExposedSceneProperty;
class ezSceneDocumentSettings;
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

class ezSceneDocument : public ezGameObjectDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocument, ezGameObjectDocument);

public:
  ezSceneDocument(const char* szDocumentPath, bool bIsPrefab);
  ~ezSceneDocument();

  virtual const char* GetDocumentTypeDisplayString() const override;


  enum class ShowOrHide
  {
    Show,
    Hide
  };


  void GroupSelection();

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

  /// \brief Creates a new empty object, either top-level (selection empty) or as a child of the selected item
  ezStatus CreateEmptyObject(bool bAttachToParent, bool bAtPickedPosition);

  void DuplicateSelection();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();

  /// \brief Whether this document represents a prefab or a scene
  bool IsPrefab() const { return m_bIsPrefab; }

  /// \brief Determines whether the given object is an editor prefab
  bool IsObjectEditorPrefab(const ezUuid& object, ezUuid* out_PrefabAssetGuid = nullptr) const;

  /// \brief Determines whether the given object is an engine prefab
  bool IsObjectEnginePrefab(const ezUuid& object, ezUuid* out_PrefabAssetGuid = nullptr) const;

  /// \brief Nested prefabs are not allowed
  virtual bool ArePrefabsAllowed() const { return !IsPrefab(); }


  virtual void GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const override;
  virtual bool Copy(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType) override;
  bool Duplicate(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bSetSelected);
  bool Copy(ezAbstractObjectGraph& graph, ezMap<ezUuid, ezUuid>* out_pParents) const;
  bool PasteAt(const ezArrayPtr<PasteInfo>& info, const ezVec3& vPos);
  bool PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info);

  virtual void UpdatePrefabs() override;

  /// \brief Removes the link to the prefab template, making the editor prefab a simple object
  virtual void UnlinkPrefabs(const ezDeque<const ezDocumentObject*>& Selection) override;

  virtual ezUuid ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed) override;

  /// \brief Reverts all selected editor prefabs to their original template state
  virtual ezUuid RevertPrefab(const ezDocumentObject* pObject) override;

  /// \brief Converts all objects in the selection that are engine prefabs to their respective editor prefab representation
  virtual void ConvertToEditorPrefab(const ezDeque<const ezDocumentObject*>& Selection);
  /// \brief Converts all objects in the selection that are editor prefabs to their respective engine prefab representation
  virtual void ConvertToEnginePrefab(const ezDeque<const ezDocumentObject*>& Selection);

  GameMode::Enum GetGameMode() const { return m_GameMode; }

  void StartSimulateWorld();
  void TriggerGameModePlay();

  /// Stops the world simulation, if it is running. Returns true, when the simulation needed to be stopped.
  bool StopGameMode();

  ezStatus ExportScene();
  void ExportSceneGeometry(const char* szFile, bool bOnlySelection, int iExtractionMode /* ezWorldGeoExtractionUtil::ExtractionMode */, const ezMat3& mTransform);

  virtual void HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  void HandleGameModeMsg(const ezGameModeMsgToEditor* pMsg);
  void HandleVisualScriptActivityMsg(const ezVisualScriptActivityMsgToEditor* pMsg);
  void HandleObjectStateFromEngineMsg(const ezPushObjectStateMsgToEditor* pMsg);
  void SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);

  /// \name Scene Settings
  ///@{

  const ezDocumentObject* GetSettingsObject() const;
  const ezSceneDocumentSettings* GetSettings() const;

  ezStatus CreateExposedProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index, ezExposedSceneProperty& out_key) const;
  ezStatus AddExposedParameter(const char* szName, const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index);
  ezInt32 FindExposedParameter(const ezDocumentObject* pObject, const ezAbstractProperty* pProperty, ezVariant index);
  ezStatus RemoveExposedParameter(ezInt32 index);
  ///@}

  /// \name Editor Camera
  ///@{

  /// \brief Stores the current editor camera position in a user preference. Slot can be 0 to 9.
  ///
  /// Since the preference is stored on disk, this position can be restored in another session.
  void StoreFavouriteCamera(ezUInt8 uiSlot);

  /// \brief Applies the previously stored camera position from slot 0 to 9 to the current camera position.
  ///
  /// The camera will quickly interpolate to the stored position.
  void RestoreFavouriteCamera(ezUInt8 uiSlot);

  ///@}

protected:
  void SetGameMode(GameMode::Enum mode);

  virtual void InitializeAfterLoading() override;
  virtual void UpdatePrefabObject(ezDocumentObject* pObject, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed, const char* szBasePrefab) override;
  virtual void UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const override;

  template<typename Func>
  void ApplyRecursive(const ezDocumentObject* pObject, Func f)
  {
    f(pObject);

    for (auto pChild : pObject->GetChildren())
    {
      ApplyRecursive<Func>(pChild, f);
    }
  }

private:

  void EnsureSettingsObjectExist();
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void EngineConnectionEventHandler(const ezEditorEngineProcessConnection::Event& e);
  void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

  ezStatus RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header);

  virtual const char* QueryAssetType() const override;

  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(ezDocumentObject* pObject);

  /// \brief Finds all objects that are actively being 'debugged' (or visualized) by the editor and thus should get the debug visualization flag in the runtime.
  void UpdateObjectDebugTargets();

  bool m_bIsPrefab;

  GameMode::Enum m_GameMode;

  GameModeData m_GameModeData[3];

  // Local mirror for settings
  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;

  //////////////////////////////////////////////////////////////////////////
  /// Communication with other document types
  virtual void OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender) override;
  void GatherObjectsOfType(ezDocumentObject* pRoot, ezGatherObjectsOfTypeMsgInterDoc* pMsg) const;

};
