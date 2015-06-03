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

  virtual bool mousePressEvent(QMouseEvent* e) override;
  virtual bool mouseReleaseEvent(QMouseEvent* e) override;
  virtual bool mouseMoveEvent(QMouseEvent* e) override;

protected:
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;

private:
  ezGizmoHandle m_AxisX;
  ezGizmoHandle m_AxisY;
  ezGizmoHandle m_AxisZ;

};
