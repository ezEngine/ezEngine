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

protected:
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

  ezVec3 GetPointOnAxis(ezInt32 iScreenPosX, ezInt32 iScreenPosY) const;
  ezVec3 GetPointOnPlane(ezInt32 iScreenPosX, ezInt32 iScreenPosY) const;

private:
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
