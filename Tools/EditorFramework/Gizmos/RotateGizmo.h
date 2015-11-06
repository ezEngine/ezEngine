#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezRotateGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmo);

public:
  ezRotateGizmo();

  virtual void FocusLost(bool bCancel) override;

  virtual ezEditorInut mousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseMoveEvent(QMouseEvent* e) override;

  const ezQuat& GetRotationResult() const { return m_CurrentRotation; }

  /// \brief Sets the angle to which to snap the rotation result to. Zero means no snapping is performed.
  void SetSnappingAngle(ezAngle angle) { m_SnappingAngle = angle; }

protected:
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
  ezAngle m_SnappingAngle;

  ezVec2 m_MousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezMat4 m_InvViewProj;
};
