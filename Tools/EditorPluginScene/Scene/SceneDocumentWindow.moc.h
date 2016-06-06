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

struct ezSceneViewPreferences;
class QGridLayout;
class ezQtSceneViewWidgetContainer;
class ezQtSceneViewWidget;
class QSettings;
struct ezManipulatorManagerEvent;

Q_DECLARE_OPAQUE_POINTER(ezQtSceneViewWidget*);

class ezQtSceneDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtSceneDocumentWindow(ezAssetDocument* pDocument);
  ~ezQtSceneDocumentWindow();

  virtual const char* GetGroupName() const { return "Scene"; }

  ezSceneDocument* GetSceneDocument() const;

public slots:
  void ToggleViews(QWidget* pView);

protected:
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;
  virtual void InternalRedraw() override;

private:
  void TransformationGizmoEventHandler(const ezGizmoEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);
  void DocumentObjectMetaDataEventHandler(const ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>::EventData& e);
  void SceneExportEventHandler(ezSceneDocumentExportEvent& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);

  void DocumentEventHandler(const ezSceneDocumentEvent& e);

  ezStatus RequestExportScene(const char* szTargetFile, const ezAssetFileHeader& header);

  void FocusOnSelectionAllViews();

  void FocusOnSelectionHoveredView();

  void SnapProviderEventHandler(const ezSnapProviderEvent& e);

  void UpdateGizmoSelectionList();
  void UpdateGizmoVisibility();
  void UpdateManipulatorVisibility();
  void UpdateGizmoPosition();
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);

  void SnapSelectionToPosition(bool bSnapEachObject);

  void SendObjectMsg(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectMsgRecursive(const ezDocumentObject* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectSelection();

  void SendRedrawMsg();

  void SaveViewConfig(const ezSceneViewConfig& cfg, ezSceneViewPreferences& pref) const;
  void LoadViewConfig(ezSceneViewConfig& cfg, ezSceneViewPreferences& pref);
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