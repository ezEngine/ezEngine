#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class EZ_EDITORFRAMEWORK_DLL ezRotateGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRotateGizmo, ezGizmo);

public:
  ezRotateGizmo();

  const ezQuat& GetRotationResult() const { return m_qCurrentRotation; }

  virtual void UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow) override;

  void EnableAxis(bool x, bool y, bool z);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

private:
  bool m_bEnableAxisX = true;
  bool m_bEnableAxisY = true;
  bool m_bEnableAxisZ = true;

  ezEngineGizmoHandle m_hAxisX;
  ezEngineGizmoHandle m_hAxisY;
  ezEngineGizmoHandle m_hAxisZ;

  ezQuat m_qStartRotation;
  ezQuat m_qCurrentRotation;
  ezAngle m_Rotation;

  ezVec2I32 m_vLastMousePos;

  ezTime m_LastInteraction;
  ezVec3 m_vRotationAxis;
  ezMat4 m_mInvViewProj;
  ezVec2 m_vScreenTangent;
};
