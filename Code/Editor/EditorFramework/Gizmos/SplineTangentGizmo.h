#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class EZ_EDITORFRAMEWORK_DLL ezSplineTangentGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSplineTangentGizmo, ezGizmo);

public:
  ezSplineTangentGizmo();

  float GetScalingResult() const { return m_fScalingResult; }
  ezQuat GetRotationResult() const;

  virtual void UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

protected:
  ezEngineGizmoHandle m_hScale;
  ezEngineGizmoHandle m_hYaw;
  ezEngineGizmoHandle m_hPitch;
  
private:
  ezAngle GetAngleFromMousePos(const ezVec2I32& vMousePos) const;

  ezUInt32 m_uiActiveHandleIndex = ezInvalidIndex;
  float m_fScalingResult = 0.0f;

  ezAngle m_fStartAngle = ezAngle::MakeZero();
  ezAngle m_fYawResult = ezAngle::MakeZero();
  ezAngle m_fPitchResult = ezAngle::MakeZero();


  ezTime m_LastInteraction;
  ezMat4 m_mInvViewProj;
};
