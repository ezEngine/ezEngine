#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmo : public ezGizmoBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmo);

public:
  ezTranslateGizmo();

  virtual void FocusLost() override;

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

  const ezVec3 GetTranslationResult() const { return GetTransformation().GetTranslationVector() - m_vStartPosition; }
  const ezVec3 GetTranslationDiff() const { return m_vLastMoveDiff; }

  /// \brief Sets the value to which to snap the scaling result to. Zero means no snapping is performed.
  void SetSnappingValue(float fSnappingValue) { m_fSnappingValue = fSnappingValue; }

  void SnapToGrid();

  enum class MovementMode
  {
    ScreenProjection,
    MouseDiff
  };

  void SetMovementMode(MovementMode mode);

protected:
  virtual void OnSetOwner(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

  ezResult GetPointOnAxis(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const;
  ezResult GetPointOnPlane(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const;

private:
  void SetCursorToWindowCenter();

  QPoint m_LastMousePos;
  QPoint m_OriginalMousePos;

  float m_fSnappingValue;
  ezVec3 m_vLastMoveDiff;

  MovementMode m_MovementMode;
  ezGizmoHandle m_AxisX;
  ezGizmoHandle m_AxisY;
  ezGizmoHandle m_AxisZ;

  ezGizmoHandle m_PlaneXY;
  ezGizmoHandle m_PlaneXZ;
  ezGizmoHandle m_PlaneYZ;

  enum class TranslateMode
  {
    None,
    Axis,
    Plane
  };

  TranslateMode m_Mode;

  float m_fStartScale;

  ezTime m_LastInteraction;
  ezVec3 m_vMoveAxis;
  ezVec3 m_vPlaneAxis[2];
  ezVec3 m_vStartPosition;
  ezMat4 m_InvViewProj;
};
