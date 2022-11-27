#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief The click gizmo displays a simple shape that can be clicked.
///
/// This can be used to provide the user with a way to select which part to edit further.
class EZ_EDITORFRAMEWORK_DLL ezClickGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClickGizmo, ezGizmo);

public:
  ezClickGizmo();

  void SetColor(const ezColor& color);

protected:
  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual void DoFocusLost(bool bCancel) override;
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

private:
  ezEngineGizmoHandle m_hShape;
};
