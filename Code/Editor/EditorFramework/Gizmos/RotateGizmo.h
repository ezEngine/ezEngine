#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezRotateGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmo, ezGizmo);

public:
  ezRotateGizmo();

  const ezQuat& GetRotationResult() const { return m_CurrentRotation; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

private:
  ezEngineGizmoHandle m_AxisX;
  ezEngineGizmoHandle m_AxisY;
  ezEngineGizmoHandle m_AxisZ;

  ezQuat m_StartRotation;
  ezQuat m_CurrentRotation;
  ezAngle m_Rotation;

  ezVec2I32 m_LastMousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vRotationAxis;
  ezMat4 m_InvViewProj;
  ezVec2 m_vScreenTangent;
};
