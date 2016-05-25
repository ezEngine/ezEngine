#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezRotateGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmo, ezGizmo);

public:
  ezRotateGizmo();

  const ezQuat& GetRotationResult() const { return m_CurrentRotation; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInut DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

private:
  ezEngineGizmoHandle m_AxisX;
  ezEngineGizmoHandle m_AxisY;
  ezEngineGizmoHandle m_AxisZ;

  ezQuat m_StartRotation;
  ezQuat m_CurrentRotation;
  ezAngle m_Rotation;

  ezVec2I32 m_LastMousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezMat4 m_InvViewProj;
};
