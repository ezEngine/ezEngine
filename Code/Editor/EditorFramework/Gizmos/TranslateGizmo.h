#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmo, ezGizmo);

public:
  ezTranslateGizmo();

  const ezVec3 GetStartPosition() const { return m_vStartPosition; }
  const ezVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const ezVec3 GetTranslationDiff() const { return m_vLastMoveDiff; }

  enum class MovementMode
  {
    ScreenProjection,
    MouseDiff
  };

  enum class HandleInteraction
  {
    None,
    AxisX,
    AxisY,
    AxisZ,
    PlaneX,
    PlaneY,
    PlaneZ,
  };

  enum class TranslateMode
  {
    None,
    Axis,
    Plane
  };

  void SetMovementMode(MovementMode mode);
  HandleInteraction GetLastHandleInteraction() const { return m_LastHandleInteraction; }
  TranslateMode GetTranslateMode() const { return m_Mode; }

  /// \brief Used when CTRL+drag moves the object AND the camera
  void SetCameraSpeed(float fSpeed);

  virtual void UpdateStatusBarText(ezQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual ezEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezTransform& transform) override;

  ezResult GetPointOnPlane(const ezVec2I32& vScreenPos, ezVec3& out_Result) const;

private:
  ezVec2I32 m_vLastMousePos;
  ezVec2 m_vTotalMouseDiff;

  ezVec3 m_vLastMoveDiff;

  MovementMode m_MovementMode;
  ezEngineGizmoHandle m_hAxisX;
  ezEngineGizmoHandle m_hAxisY;
  ezEngineGizmoHandle m_hAxisZ;

  ezEngineGizmoHandle m_hPlaneXY;
  ezEngineGizmoHandle m_hPlaneXZ;
  ezEngineGizmoHandle m_hPlaneYZ;

  TranslateMode m_Mode;
  HandleInteraction m_LastHandleInteraction;

  float m_fStartScale;
  float m_fCameraSpeed;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezVec3 m_vPlaneAxis[2];
  ezVec3 m_vStartPosition;
  ezMat4 m_mInvViewProj;
};
