#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class EZ_EDITORFRAMEWORK_DLL ezTranslateGizmo : public ezGizmoBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTranslateGizmo);

public:
  ezTranslateGizmo();

  virtual void SetDocumentGuid(const ezUuid& guid) override;

  virtual void FocusLost() override;

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

  const ezVec3 GetTranslationResult() const { return GetTransformation().GetTranslationVector() - m_vStartPosition; }

  /// \brief Sets the value to which to snap the scaling result to. Zero means no snapping is performed.
  void SetSnappingValue(float fSnappingValue) { m_fSnappingValue = fSnappingValue; }

  void SnapToGrid();

protected:
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

  ezResult GetPointOnAxis(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const;
  ezResult GetPointOnPlane(ezInt32 iScreenPosX, ezInt32 iScreenPosY, ezVec3& out_Result) const;

private:
  float m_fSnappingValue;

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
  ezVec3 m_vStartPosition;
  ezMat4 m_InvViewProj;
};
