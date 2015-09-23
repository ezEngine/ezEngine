#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezScaleGizmo : public ezGizmoBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmo);

public:
  ezScaleGizmo();

  virtual void FocusLost() override;

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

  const ezVec3& GetScalingResult() const { return m_vScalingResult; }

  /// \brief Sets the value to which to snap the scaling result to. Zero means no snapping is performed.
  void SetSnappingValue(float fSnappingValue) { m_fSnappingValue = fSnappingValue; }

protected:
  virtual void OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

private:
  float m_fSnappingValue;

  ezVec3 m_vScalingResult;
  ezVec3 m_vScaleMouseMove;

  ezGizmoHandle m_AxisX;
  ezGizmoHandle m_AxisY;
  ezGizmoHandle m_AxisZ;
  ezGizmoHandle m_AxisXYZ;

  ezVec2 m_MousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezMat4 m_InvViewProj;
};
