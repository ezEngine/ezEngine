#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezDrawBoxGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDrawBoxGizmo, ezGizmo);

public:
  ezDrawBoxGizmo();

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoKeyPressEvent(QKeyEvent* e) override;
  virtual ezEditorInut DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

private:
  enum class ManipulateMode
  {
    None,
    DrawBase,
    DrawHeight,
  };

  ManipulateMode m_ManipulateMode;
  ezEngineGizmoHandle m_Box;
};
