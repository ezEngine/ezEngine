#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <CoreUtils/Graphics/Camera.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <EditorPluginScene/InputContexts/CameraMoveContext.h>
#include <EditorPluginScene/InputContexts/CameraPositionContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/UniquePtr.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorPluginScene/Actions/GizmoActions.h>

class ezScene3DWidget : public ez3DViewWidget
{
public:
  ezScene3DWidget(QWidget* pParent, ezDocumentWindow3D* pDocument);

protected:
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

  ezUuid CreateDropObject(const ezVec3& vPosition, const char* szType, const char* szProperty, const char* szValue);
  void MoveObjectToPosition(const ezUuid& guid, const ezVec3& vPosition);
  void MoveDraggedObjectsToPosition(const ezVec3& vPosition);

  ezHybridArray<ezUuid, 16> m_DraggedObjects;
  ezTime m_LastDragMoveEvent;
};

class ezSceneDocumentWindow : public ezDocumentWindow3D
{
  Q_OBJECT

public:
  ezSceneDocumentWindow(ezDocumentBase* pDocument);
  ~ezSceneDocumentWindow();

  virtual const char* GetGroupName() const { return "Scene"; }

  ezSceneDocument* GetSceneDocument() const { return static_cast<ezSceneDocument*>(GetDocument()); }

private slots:


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

  ezScene3DWidget* m_pCenterWidget;

  ezSelectionContext* m_pSelectionContext;
  ezCameraMoveContext* m_pCameraMoveContext;
  ezCameraPositionContext* m_pCameraPositionContext;

  ezTranslateGizmo m_TranslateGizmo;
  ezRotateGizmo m_RotateGizmo;
  ezScaleGizmo m_ScaleGizmo;
  ezDragToPositionGizmo m_DragToPosGizmo;

  ezCamera m_Camera;

  struct SelectedGO
  {
    const ezDocumentObjectBase* m_pObject;
    ezVec3 m_vLocalScaling;
    ezTransform m_GlobalTransform;
  };

  bool m_bResendSelection;
  bool m_bInGizmoInteraction;
  bool m_bMergeTransactions;
  ezDeque<SelectedGO> m_GizmoSelection;

  bool m_bInDragDropAction;
};