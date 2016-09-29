#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <CoreUtils/DataStructures/ObjectMetaData.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>

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
    SimulationSpeedChanged,
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

  /// \brief Creates a new empty node, either top-level (selection empty) or as a child of the selected item
  void CreateEmptyNode(bool bAttachToParent, bool bAtPickedPosition);

  void DuplicateSelection();
  void ShowOrHideSelectedObjects(ShowOrHide action);
  void ShowOrHideAllObjects(ShowOrHide action);
  void HideUnselectedObjects();

  bool IsPrefab() const { return m_bIsPrefab; }

  void SetGizmoWorldSpace(bool bWorldSpace) const;
  bool GetGizmoWorldSpace() const;

  virtual bool Copy(ezAbstractObjectGraph& out_objectGraph) const override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition) override;
  bool Duplicate(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bSetSelected);
  bool Copy(ezAbstractObjectGraph& graph, ezMap<ezUuid, ezUuid>* out_pParents) const;
  bool PasteAt(const ezArrayPtr<PasteInfo>& info, const ezVec3& vPos);
  bool PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info);

  virtual void UpdatePrefabs() override;
  virtual void UnlinkPrefabs(const ezDeque<const ezDocumentObject*>& Selection) override;
  virtual ezUuid ReplaceByPrefab(const ezDocumentObject* pRootObject, const char* szPrefabFile, const ezUuid& PrefabAsset, const ezUuid& PrefabSeed) override;
  virtual ezUuid RevertPrefab(const ezDocumentObject* pObject) override;

  /// \brief Sets the new global transformation of the given object.
  /// The transformationChanges bitmask (of type TransformationChanges) allows to tell the system that, e.g. only translation has changed and thus some work can be spared.
  void SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 transformationChanges) const;
  const ezTransform& GetGlobalTransform(const ezDocumentObject* pObject) const;

  static ezTransform QueryLocalTransform(const ezDocumentObject* pObject);
  ezTransform ComputeGlobalTransform(const ezDocumentObject* pObject) const;

  mutable ezEvent<const ezSceneDocumentEvent&> m_SceneEvents;
  ezObjectMetaData<ezUuid, ezSceneObjectMetaData> m_SceneObjectMetaData;

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


  ezStatus ExportScene();

  /// \brief Traverses the pObject hierarchy up until it hits an ezGameObject, then computes the global transform of that.
  virtual ezResult ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const override;
  
  virtual void HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  void HandleGameModeMsg(const ezGameModeMsgToEditor* pMsg);
  void SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectSelection();

protected:
  void SetGameMode(GameMode::Enum mode);

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

  virtual ezStatus InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
  virtual ezStatus InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform, const ezAssetFileHeader& AssetHeader) override;
  ezStatus InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader) override;

  virtual ezStatus InternalRetrieveAssetInfo(const char* szPlatform) override;

  void SyncObjectHiddenState();
  void SyncObjectHiddenState(ezDocumentObject* pObject);

  struct GameModeData
  {
    bool m_bRenderSelectionOverlay;
    bool m_bRenderVisualizers;
    bool m_bRenderShapeIcons;
  };

  bool m_bIsPrefab;
  mutable bool m_bGizmoWorldSpace; // whether the gizmo is in local/global space mode
  GameMode::Enum m_GameMode;
  float m_fSimulationSpeed;

  // when new objects are created the engine sometimes needs to catch up creating sub-objects (e.g. for reference prefabs)
  // therefore when the selection is changed in the first frame, it might not be fully correct
  // by sending it a second time, we can fix that easily
  ezInt8 m_iResendSelection;

  GameModeData m_CurrentMode;
  GameModeData m_GameModeData[3];

  mutable ActiveGizmo m_ActiveGizmo;

  mutable ezHashTable<const ezDocumentObject*, ezTransform> m_GlobalTransforms;


};
