#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezScaleGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezScaleGizmo, ezGizmo);

public:
  ezScaleGizmo();

  virtual void FocusLost(bool bCancel) override;

  virtual ezEditorInut mousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut mouseMoveEvent(QMouseEvent* e) override;

  const ezVec3& GetScalingResult() const { return m_vScalingResult; }

  /// \brief Sets the value to which to snap the scaling result to. Zero means no snapping is performed.
  void SetSnappingValue(float fSnappingValue) { m_fSnappingValue = fSnappingValue; }

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

private:
  float m_fSnappingValue;

  ezVec3 m_vScalingResult;
  ezVec3 m_vScaleMouseMove;

  ezEngineGizmoHandle m_AxisX;
  ezEngineGizmoHandle m_AxisY;
  ezEngineGizmoHandle m_AxisZ;
  ezEngineGizmoHandle m_AxisXYZ;

  ezVec2 m_MousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezMat4 m_InvViewProj;
};
