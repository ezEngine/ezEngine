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
class ezSceneViewWidgetContainer;
class ezSceneViewWidget;

Q_DECLARE_OPAQUE_POINTER(ezSceneViewWidget*);

class ezSceneDocumentWindow : public ezDocumentWindow3D
{
  Q_OBJECT

public:
  ezSceneDocumentWindow(ezDocumentBase* pDocument);
  ~ezSceneDocumentWindow();

  virtual const char* GetGroupName() const { return "Scene"; }

  ezSceneDocument* GetSceneDocument() const { return static_cast<ezSceneDocument*>(GetDocument()); }

public slots:
  void ToggleViews(QWidget* pView);

private:
  void TransformationGizmoEventHandler(const ezGizmoBase::BaseEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManager::Event& e);

  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  virtual void InternalRedraw() override;
  void DocumentEventHandler(const ezSceneDocument::SceneEvent& e);
  void DocumentTreeEventHandler(const ezDocumentObjectStructureEvent& e);
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void RotateGizmoEventHandler(const ezRotateGizmoAction::Event& e);
  void ScaleGizmoEventHandler(const ezScaleGizmoAction::Event& e);
  void TranslateGizmoEventHandler(const ezTranslateGizmoAction::Event& e);

  void UpdateGizmoSelectionList();
  void UpdateGizmoVisibility();
  void UpdateGizmoPosition();
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistory::Event& e);

  void SnapSelectionToPosition(bool bSnapEachObject);
  void HideSelectedObjects(bool bHide);
  void HideUnselectedObjects();
  void ShowHiddenObjects();

  void SendObjectMsgRecursive(const ezDocumentObjectBase* pObj, ezObjectTagMsgToEngine* pMsg);
  void SendObjectSelection();

  void SendRedrawMsg();

  void SetupDefaultViewConfigs();
  void CreateViews(bool bQuad);

  ezSceneViewConfig m_ViewConfigSingle;
  ezSceneViewConfig m_ViewConfigQuad[4];
  ezHybridArray<ezSceneViewWidgetContainer*, 4> m_ActiveMainViews;

  ezTranslateGizmo m_TranslateGizmo;
  ezRotateGizmo m_RotateGizmo;
  ezScaleGizmo m_ScaleGizmo;
  ezDragToPositionGizmo m_DragToPosGizmo;

  ezCameraMoveContextSettings m_CameraMoveSettings;
  ezVec3 m_vLastTranslationGizmoResult;

  struct SelectedGO
  {
    const ezDocumentObjectBase* m_pObject;
    ezVec3 m_vLocalScaling;
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