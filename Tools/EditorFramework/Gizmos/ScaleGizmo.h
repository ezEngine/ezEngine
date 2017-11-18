#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezScaleGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmo, ezGizmo);

public:
  ezScaleGizmo();

  const ezVec3& GetScalingResult() const { return m_vScalingResult; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

private:
  ezVec3 m_vScalingResult;
  ezVec3 m_vScaleMouseMove;

  ezEngineGizmoHandle m_AxisX;
  ezEngineGizmoHandle m_AxisY;
  ezEngineGizmoHandle m_AxisZ;
  ezEngineGizmoHandle m_AxisXYZ;

  ezVec2I32 m_LastMousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezMat4 m_InvViewProj;
};
