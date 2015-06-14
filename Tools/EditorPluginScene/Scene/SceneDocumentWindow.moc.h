#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/IPC/ProcessCommunication.h>
#include <EditorFramework/EngineProcess/EngineProcessConnection.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <CoreUtils/Graphics/Camera.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <EditorPluginScene/InputContexts/CameraMoveContext.h>
#include <Foundation/Types/UniquePtr.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>

class ezSceneDocumentWindow : public ezDocumentWindow3D
{
  Q_OBJECT

public:
  ezSceneDocumentWindow(ezDocumentBase* pDocument);
  ~ezSceneDocumentWindow();

  virtual const char* GetGroupName() const { return "Scene"; }

  private slots:


private:
  void TransformationGizmoEventHandler(const ezGizmoBase::BaseEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManager::Event& e);

  virtual bool HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  virtual void InternalRedraw() override;
  void DocumentTreeEventHandler(const ezDocumentObjectStructureEvent& e);
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

  void SendRedrawMsg();

  ez3DViewWidget* m_pCenterWidget;

  ezSelectionContext* m_pSelectionContext;
  ezCameraMoveContext* m_pMoveContext;

  ezTranslateGizmo m_TranslateGizmo;
  ezRotateGizmo m_RotateGizmo;
  ezScaleGizmo m_ScaleGizmo;

  ezCamera m_Camera;

  struct SelectedGO
  {
    ezUuid m_Object;
    ezVec3 m_vTranslation;
    ezVec3 m_vScaling;
    ezQuat m_Rotation;
  };

  ezDeque<SelectedGO> m_GizmoSelection;

  /// \todo Broken delegates
  ezDelegate<void(const ezDocumentObjectPropertyEvent&)> m_DelegatePropertyEvents;
  ezDelegate<void(const ezDocumentObjectStructureEvent&)> m_DelegateDocumentTreeEvents;
};