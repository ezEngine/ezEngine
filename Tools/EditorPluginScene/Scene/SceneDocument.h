#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/SimdMath/SimdTransform.h>

class ezAssetFileHeader;

enum class ActiveGizmo
{
  None,
  Translate,
  Rotate,
  Scale,
  DragToPosition,
};

struct GameMode
{
  enum Enum
  {
    Off,
    Simulate,
    Play,
  };
};

struct TransformationChanges
{
  enum Enum
  {
    Translation = EZ_BIT(0),
    Rotation = EZ_BIT(1),
    Scale = EZ_BIT(2),
    UniformScale = EZ_BIT(3),
    All = 0xFF
  };
};

struct ezSceneDocumentEvent
{
  enum class Type
  {
    ActiveGizmoChanged,
    ShowSelectionInScenegraph,
    FocusOnSelection_Hovered,
    FocusOnSelection_All,
    SnapSelectionPivotToGrid,
    SnapEachSelectedObjectToGrid,
    GameModeChanged,
    RenderSelectionOverlayChanged,
    RenderVisualizersChanged,
    RenderShapeIconsChanged,
    AddAmbientLightChanged,
    SimulationSpeedChanged,
    BeforeTriggerGameModePlay,
    TriggerGameModePlay,
    TriggerStopGameModePlay,
  };

  Type m_Type;
};

class ezSceneObjectMetaData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneObjectMetaData, ezReflectedClass);

public:

  enum ModifiedFlags
  {
    CachedName = EZ_BIT(2),

    AllFlags = 0xFFFFFFFF
  };

  ezSceneObjectMetaData()
  {
  }

  ezString m_CachedNodeName;
  QIcon m_Icon;
};

class ezSceneDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocument, ezAssetDocument);

public:
  ezSceneDocument(const char* szDocumentPath, bool bIsPrefab);
  ~ezSceneDocument();

  virtual const char* GetDocumentTypeDisplayString() const override;

  void SetActiveGizmo(ActiveGizmo gizmo) const;
  ActiveGizmo GetActiveGizmo() const;

  enum class ShowOrHide
  {
    Show,
    Hide
  };

  void TriggerShowSelectionInScenegraph() const;
  void TriggerFocusOnSelection(bool bAllViews) const;
  void TriggerSnapPivotToGrid() const;
  void TriggerSnapEachObjectToGrid() const;
  void GroupSelection();

  /// \brief Opens the Duplicate Special dialog
  void DuplicateSpecial();

  /// \brief Moves the editor camera to the same position as the selected object
  void SnapCameraToObject();

  /// \brief Moves all selected objects to the editor camera position
  void SnapObjectToCamera();

  /// \brief Moves the camera to the current picking position
  void MoveCameraHere();

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

  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

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

  /// \brief Sets the new global transformation of the given object.
  /// The transformationChanges bitmask (of type TransformationChanges) allows to tell the system that, e.g. only translation has changed and thus some work can be spared.
  void SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 transformationChanges) const;

  /// \brief Returns a cached value for the global transform of the given object, if available. Otherwise it calls ComputeGlobalTransform().
  ezTransform GetGlobalTransform(const ezDocumentObject* pObject) const;

  /// \brief Retrieves the local transform property values from the object and combines it into one ezTransform
  static ezTransform QueryLocalTransform(const ezDocumentObject* pObject);
  static ezSimdTransform QueryLocalTransformSimd(const ezDocumentObject* pObject);

  /// \brief Computes the global transform of the parent and combines it with the local transform of the given object.
  /// This function does not return a cached value, but always computes it. It does update the internal cache for later reads though.
  ezTransform ComputeGlobalTransform(const ezDocumentObject* pObject) const;

  mutable ezEvent<const ezSceneDocumentEvent&> m_SceneEvents;
  mutable ezObjectMetaData<ezUuid, ezSceneObjectMetaData> m_SceneObjectMetaData;

  GameMode::Enum GetGameMode() const { return m_GameMode; }

  void StartSimulateWorld();
  void TriggerGameModePlay();

  /// Stops the world simulation, if it is running. Returns true, when the simulation needed to be stopped.
  bool StopGameMode();

  float GetSimulationSpeed() const { return m_fSimulationSpeed; }
  void SetSimulationSpeed(float f);

  bool GetRenderSelectionOverlay() const { return m_CurrentMode.m_bRenderSelectionOverlay; }
  void SetRenderSelectionOverlay( bool b );

  bool GetRenderVisualizers() const { return m_CurrentMode.m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  bool GetRenderShapeIcons() const { return m_CurrentMode.m_bRenderShapeIcons; }
  void SetRenderShapeIcons(bool b);

  bool GetAddAmbientLight() const { return m_bAddAmbientLight; }
  void SetAddAmbientLight(bool b);


  ezStatus ExportScene();

  /// \brief Traverses the pObject hierarchy up until it hits an ezGameObject, then computes the global transform of that.
  virtual ezResult ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const override;

  virtual void HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  void HandleGameModeMsg(const ezGameModeMsgToEditor* pMsg);
  void HandleVisualScriptActivityMsg(const ezVisualScriptActivityMsgToEditor* pMsg);
  void SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectSelection();

  /// \brief Generates a good name for pObject. Queries the "Name" property, child components and asset properties, if necessary.
  void DetermineNodeName(const ezDocumentObject* pObject, const ezUuid& prefabGuid, ezStringBuilder& out_Result, QIcon* out_pIcon = nullptr) const;

  /// \brief Similar to DetermineNodeName() but prefers to return the last cached value from scene meta data. This is more efficient, but may give in an outdated result.
  void QueryCachedNodeName(const ezDocumentObject* pObject, ezStringBuilder& out_Result, ezUuid* out_pPrefabGuid = nullptr, QIcon* out_pIcon = nullptr) const;

  /// \brief Creates a full "path" to a scene object for display in UIs. No guarantee for uniqueness.
  void GenerateFullDisplayName(const ezDocumentObject* pRoot, ezStringBuilder& out_sFullPath) const;

protected:
  void SetGameMode(GameMode::Enum mode);

  /// \brief Sends the current state of the scene to the engine process. This is typically done after scene load or when the world might have deviated on the engine side (after play the game etc.)
  void SendGameWorldToEngine();

  virtual void InitializeAfterLoading() override;

  template<typename Func>
  void ApplyRecursive(const ezDocumentObject* pObject, Func f)
  {
    f(pObject);

    for (auto pChild : pObject->GetChildren())
    {
      ApplyRecursive<Func>(pChild, f);
    }
  }

  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph) override;

private:
  void ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void ObjectEventHandler(const ezDocumentObjectEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void EngineConnectionEventHandler(const ezEditorEngineProcessConnection::Event& e);
  void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

  ezStatus RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header);

  void InvalidateGlobalTransformValue(const ezDocumentObject* pObject) const;

  virtual const char* QueryAssetType() const override;

  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually) override;
  ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(ezDocumentObject* pObject);

  /// \brief Finds all objects that are actively being 'debugged' (or visualized) by the editor and thus should get the debug visualization flag in the runtime.
  void UpdateObjectDebugTargets();

  struct GameModeData
  {
    bool m_bRenderSelectionOverlay;
    bool m_bRenderVisualizers;
    bool m_bRenderShapeIcons;
  };

  bool m_bIsPrefab;
  bool m_bAddAmbientLight;
  bool m_bGizmoWorldSpace; // whether the gizmo is in local/global space mode
  GameMode::Enum m_GameMode;
  float m_fSimulationSpeed;

  // when new objects are created the engine sometimes needs to catch up creating sub-objects (e.g. for reference prefabs)
  // therefore when the selection is changed in the first frame, it might not be fully correct
  // by sending it a second time, we can fix that easily
  ezInt8 m_iResendSelection;

  GameModeData m_CurrentMode;
  GameModeData m_GameModeData[3];

  mutable ActiveGizmo m_ActiveGizmo;

  typedef ezHashTable<const ezDocumentObject*, ezSimdTransform, ezHashHelper<const ezDocumentObject*>, ezAlignedAllocatorWrapper> TransformTable;
  mutable TransformTable m_GlobalTransforms;

  //////////////////////////////////////////////////////////////////////////
  /// Communication with other document types
  virtual void OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender) override;
  void GatherObjectsOfType(ezDocumentObject* pRoot, ezGatherObjectsOfTypeMsgInterDoc* pMsg) const;

};
