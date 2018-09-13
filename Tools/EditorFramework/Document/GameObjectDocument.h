#pragma once
#include <EditorFramework/Plugin.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class ezAssetFileHeader;
class ezGameObjectEditTool;

struct EZ_EDITORFRAMEWORK_DLL TransformationChanges
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

struct EZ_EDITORFRAMEWORK_DLL ezGameObjectEvent
{
  enum class Type
  {
    RenderSelectionOverlayChanged,
    RenderVisualizersChanged,
    RenderShapeIconsChanged,
    AddAmbientLightChanged,
    SimulationSpeedChanged,

    ActiveEditToolChanged,

    TriggerShowSelectionInScenegraph,
    TriggerFocusOnSelection_Hovered,
    TriggerFocusOnSelection_All,

    TriggerSnapSelectionPivotToGrid,
    TriggerSnapEachSelectedObjectToGrid,

    GameModeChanged,
    BeforeTriggerGameModePlay,
    TriggerGameModePlay,
    TriggerStopGameModePlay,

    GizmoTransformMayBeInvalid, ///< Sent when a change was made that may affect the current gizmo / manipulator state (ie. objects have been moved)
  };

  Type m_Type;
};

class EZ_EDITORFRAMEWORK_DLL ezGameObjectMetaData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectMetaData, ezReflectedClass);

public:

  enum ModifiedFlags
  {
    CachedName = EZ_BIT(2),
    AllFlags = 0xFFFFFFFF
  };

  ezGameObjectMetaData()
  {
  }

  ezString m_CachedNodeName;
  QIcon m_Icon;
};

struct EZ_EDITORFRAMEWORK_DLL ezSelectedGameObject
{
  const ezDocumentObject* m_pObject;
  ezVec3 m_vLocalScaling;
  float m_fLocalUniformScaling;
  ezTransform m_GlobalTransform;
};

class EZ_EDITORFRAMEWORK_DLL ezGameObjectDocument : public ezAssetDocument
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectDocument, ezAssetDocument);
public:
  ezGameObjectDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, bool bUseEngineConnection = true, bool bUseIPCObjectMirror = true);
  ~ezGameObjectDocument();

  virtual ezEditorInputContext* GetEditorInputContextOverride() override;

  /// \name Gizmo
  ///@{
public:

  /// \brief Makes an edit tool of the given type active. Allocates a new one, if necessary. Only works when SetEditToolConfigDelegate() is set.
  void SetActiveEditTool(const ezRTTI* pEditToolType);

  /// \brief Returns the currently active edit tool (nullptr for none).
  ezGameObjectEditTool* GetActiveEditTool() const { return m_pActiveEditTool; }

  /// \brief Checks whether an edit tool of the given type, or nullptr for none, is active.
  bool IsActiveEditTool(const ezRTTI* pEditToolType) const;

  /// \brief Needs to be called by some higher level code (usually the DocumentWindow) to react to newly created edit tools to configure them (call ezGameObjectEditTool::ConfigureTool()).
  void SetEditToolConfigDelegate(ezDelegate<void(ezGameObjectEditTool*)> configDelegate);

  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  void SetGizmoMoveParentOnly(bool bMoveParent);
  bool GetGizmoMoveParentOnly() const;

  /// \brief Finds all objects that are selected at the top level, ie. none of their parents is selected.
  ///
  /// Additionally stores the current transformation. Useful to store this at the start of an operation
  /// to then do modifications on this base transformation every frame.
  void ComputeTopLevelSelectedGameObjects(ezDeque<ezSelectedGameObject>& out_Selection);

  virtual void HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;

private:
  void DeallocateEditTools();

  ezDelegate<void(ezGameObjectEditTool*)> m_EditToolConfigDelegate;
  ezGameObjectEditTool* m_pActiveEditTool = nullptr;
  ezMap<const ezRTTI*, ezGameObjectEditTool*> m_CreatedEditTools;


  ///@}
  /// \name Actions
  ///@{

public:
  void TriggerShowSelectionInScenegraph() const;
  void TriggerFocusOnSelection(bool bAllViews) const;
  void TriggerSnapPivotToGrid() const;
  void TriggerSnapEachObjectToGrid() const;
  /// \brief Moves the editor camera to the same position as the selected object
  void SnapCameraToObject();
  /// \brief Moves the camera to the current picking position
  void MoveCameraHere();

  void ScheduleSendObjectSelection();

  /// \brief Sends the current object selection, but only if it was modified or specifically tagged for resending with ScheduleSendObjectSelection().
  void SendObjectSelection();

  ///@}
  /// \name Settings
  ///@{
public:
  bool GetAddAmbientLight() const { return m_bAddAmbientLight; }
  void SetAddAmbientLight(bool b);

  float GetSimulationSpeed() const { return m_fSimulationSpeed; }
  void SetSimulationSpeed(float f);

  bool GetRenderSelectionOverlay() const { return m_CurrentMode.m_bRenderSelectionOverlay; }
  void SetRenderSelectionOverlay(bool b);

  bool GetRenderVisualizers() const { return m_CurrentMode.m_bRenderVisualizers; }
  void SetRenderVisualizers(bool b);

  bool GetRenderShapeIcons() const { return m_CurrentMode.m_bRenderShapeIcons; }
  void SetRenderShapeIcons(bool b);

  ///@}
  /// \name Transform
  ///@{

  /// \brief Sets the new global transformation of the given object.
  /// The transformationChanges bitmask (of type TransformationChanges) allows to tell the system that, e.g. only translation has changed and thus some work can be spared.
  void SetGlobalTransform(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 transformationChanges) const;

  /// \brief Same as SetGlobalTransform, except that all children will keep their current global transform (thus their local transforms are adjusted)
  void SetGlobalTransformParentOnly(const ezDocumentObject* pObject, const ezTransform& t, ezUInt8 transformationChanges) const;

  /// \brief Returns a cached value for the global transform of the given object, if available. Otherwise it calls ComputeGlobalTransform().
  ezTransform GetGlobalTransform(const ezDocumentObject* pObject) const;

  /// \brief Retrieves the local transform property values from the object and combines it into one ezTransform
  static ezTransform QueryLocalTransform(const ezDocumentObject* pObject);
  static ezSimdTransform QueryLocalTransformSimd(const ezDocumentObject* pObject);

  /// \brief Computes the global transform of the parent and combines it with the local transform of the given object.
  /// This function does not return a cached value, but always computes it. It does update the internal cache for later reads though.
  ezTransform ComputeGlobalTransform(const ezDocumentObject* pObject) const;

  /// \brief Traverses the pObject hierarchy up until it hits an ezGameObject, then computes the global transform of that.
  virtual ezResult ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_Result) const override;

  ///@}
  /// \name Node Names
  ///@{

  /// \brief Generates a good name for pObject. Queries the "Name" property, child components and asset properties, if necessary.
  void DetermineNodeName(const ezDocumentObject* pObject, const ezUuid& prefabGuid, ezStringBuilder& out_Result, QIcon* out_pIcon = nullptr) const;

  /// \brief Similar to DetermineNodeName() but prefers to return the last cached value from scene meta data. This is more efficient, but may give an outdated result.
  void QueryCachedNodeName(const ezDocumentObject* pObject, ezStringBuilder& out_Result, ezUuid* out_pPrefabGuid = nullptr, QIcon* out_pIcon = nullptr) const;

  /// \brief Creates a full "path" to a scene object for display in UIs. No guarantee for uniqueness.
  void GenerateFullDisplayName(const ezDocumentObject* pRoot, ezStringBuilder& out_sFullPath) const;

  ///@}

public:
  mutable ezEvent<const ezGameObjectEvent&> m_GameObjectEvents;
  mutable ezObjectMetaData<ezUuid, ezGameObjectMetaData> m_GameObjectMetaData;

protected:
  void InvalidateGlobalTransformValue(const ezDocumentObject* pObject) const;
  /// \brief Sends the current state of the scene to the engine process. This is typically done after scene load or when the world might have deviated on the engine side (after play the game etc.)
  void SendGameWorldToEngine();

  virtual void InitializeAfterLoading() override;
  virtual void AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable) override;

private:
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);
  void ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void ObjectEventHandler(const ezDocumentObjectEvent& e);

protected:
  struct EZ_EDITORFRAMEWORK_DLL GameModeData
  {
    bool m_bRenderSelectionOverlay;
    bool m_bRenderVisualizers;
    bool m_bRenderShapeIcons;
  };
  GameModeData m_CurrentMode;

private:
  bool m_bAddAmbientLight = false;
  bool m_bGizmoWorldSpace = true; // whether the gizmo is in local/global space mode
  bool m_bGizmoMoveParentOnly = false;

  float m_fSimulationSpeed = 1.0f;

  typedef ezHashTable<const ezDocumentObject*, ezSimdTransform, ezHashHelper<const ezDocumentObject*>, ezAlignedAllocatorWrapper> TransformTable;
  mutable TransformTable m_GlobalTransforms;

  // when new objects are created the engine sometimes needs to catch up creating sub-objects (e.g. for reference prefabs)
  // therefore when the selection is changed in the first frame, it might not be fully correct
  // by sending it a second time, we can fix that easily
  ezInt8 m_iResendSelection = 0;
};
