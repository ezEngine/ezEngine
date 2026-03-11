#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class EZ_EDITORFRAMEWORK_DLL ezScaleGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmo, ezGizmo);

public:
  ezScaleGizmo();

  const ezVec3& GetScalingResult() const { return m_vScalingResult; }

  virtual void UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow) override;

  void EnableAxis(bool x, bool y, bool z, bool bXyz);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

protected:
  bool m_bEnableAxisX = true;
  bool m_bEnableAxisY = true;
  bool m_bEnableAxisZ = true;
  bool m_bEnableAxisXYZ = true;

  ezEngineGizmoHandle m_hAxisX;
  ezEngineGizmoHandle m_hAxisY;
  ezEngineGizmoHandle m_hAxisZ;
  ezEngineGizmoHandle m_hAxisXYZ;

private:
  ezVec3 m_vScalingResult;
  ezVec3 m_vScaleMouseMove;

  ezVec2I32 m_vLastMousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezMat4 m_mInvViewProj;
};

/// \brief Scale gizmo version that only uses boxes that can be composited with
/// rotate and translate gizmos without major overlap.
/// Used by the ezTransformManipulatorAdapter.
class EZ_EDITORFRAMEWORK_DLL ezManipulatorScaleGizmo : public ezScaleGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezManipulatorScaleGizmo, ezScaleGizmo);

public:
  ezManipulatorScaleGizmo();

protected:
  virtual void OnTransformationChanged(const ezTransform& transform) override;
};
