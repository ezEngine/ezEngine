#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <EditorPluginScene/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorPluginScene/Actions/GizmoActions.h>

class QGridLayout;
class ezQtSceneViewWidgetContainer;
class ezQtSceneViewWidget;
class QSettings;

Q_DECLARE_OPAQUE_POINTER(ezQtSceneViewWidget*);

class ezQtSceneDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtSceneDocumentWindow(ezDocument* pDocument);
  ~ezQtSceneDocumentWindow();

  virtual const char* GetGroupName() const { return "Scene"; }

  ezSceneDocument* GetSceneDocument() const { return static_cast<ezSceneDocument*>(GetDocument()); }

public slots:
  void ToggleViews(QWidget* pView);

private:
  void TransformationGizmoEventHandler(const ezGizmoEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManager::Event& e);
  void SceneObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezSceneObjectMetaData>::EventData& e);
  void SceneExportEventHandler(ezSceneDocumentExportEvent& e);

  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  virtual void InternalRedraw() override;
  void DocumentEventHandler(const ezSceneDocumentEvent& e);

  ezStatus RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header);

  void FocusOnSelectionAllViews();

  void FocusOnSelectionHoveredView();

  void RotateGizmoEventHandler(const ezRotateGizmoAction::Event& e);
  void ScaleGizmoEventHandler(const ezScaleGizmoAction::Event& e);
  void TranslateGizmoEventHandler(const ezTranslateGizmoAction::Event& e);

  void UpdateGizmoSelectionList();
  void UpdateGizmoVisibility();
  void UpdateGizmoPosition();
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistory::Event& e);

  void SnapSelectionToPosition(bool bSnapEachObject);

  void SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectSelection();

  void SendRedrawMsg();

  void SetupDefaultViewConfigs();
  void SaveViewConfig(QSettings& Settings, const ezSceneViewConfig& cfg, const char* szName) const;
  void LoadViewConfig(QSettings& Settings, ezSceneViewConfig& cfg, const char* szName);
  void SaveViewConfigs() const;
  void LoadViewConfigs();
  void CreateViews(bool bQuad);

  void HandleFocusOnSelection(const ezQuerySelectionBBoxResultMsgToEditor* pMsg, ezQtSceneViewWidget* pSceneView);
  void SyncObjectHiddenState();
  void SyncObjectHiddenState(ezDocumentObject* pObject);

  ezSceneViewConfig m_ViewConfigSingle;
  ezSceneViewConfig m_ViewConfigQuad[4];
  ezHybridArray<ezQtSceneViewWidgetContainer*, 4> m_ActiveMainViews;

  ezTranslateGizmo m_TranslateGizmo;
  ezRotateGizmo m_RotateGizmo;
  ezScaleGizmo m_ScaleGizmo;
  ezDragToPositionGizmo m_DragToPosGizmo;

  ezCameraMoveContextSettings m_CameraMoveSettings;
  ezVec3 m_vLastTranslationGizmoResult;

  struct SelectedGO
  {
    const ezDocumentObject* m_pObject;
    ezVec3 m_vLocalScaling;
    float m_fLocalUniformScaling;
    ezTransform m_GlobalTransform;
  };

  bool m_bResendSelection;
  bool m_bInGizmoInteraction;
  bool m_bMergeTransactions;
  bool m_bMoveCameraWithGizmo;
  ezDeque<SelectedGO> m_GizmoSelection;

  bool m_bInDragDropAction;

  QGridLayout* m_pViewLayout;
};