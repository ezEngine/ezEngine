#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezRotateGizmo : public ezGizmoBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmo);

public:
  ezRotateGizmo();

  virtual void FocusLost() override;

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

  const ezQuat& GetRotationResult() const { return m_CurrentRotation; }

  /// \brief Sets the angle to which to snap the rotation result to. Zero means no snapping is performed.
  void SetSnappingAngle(ezAngle angle) { m_SnappingAngle = angle; }

protected:
  virtual void OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

private:
  ezGizmoHandle m_AxisX;
  ezGizmoHandle m_AxisY;
  ezGizmoHandle m_AxisZ;

  ezQuat m_StartRotation;
  ezQuat m_CurrentRotation;
  ezAngle m_Rotation;
  ezAngle m_SnappingAngle;

  ezVec2 m_MousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezMat4 m_InvViewProj;
};
