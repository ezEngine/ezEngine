#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class EZ_EDITORFRAMEWORK_DLL ezBoxGizmo : public ezGizmo
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoxGizmo, ezGizmo);

public:
  ezBoxGizmo();

  virtual void FocusLost(bool bCancel) override;

  void SetSize(const ezVec3& size);

  const ezVec3& GetSize() const { return m_vSize; }

protected:
  virtual ezEditorInut doMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut doMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut doMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const ezMat4& transform) override;


private:
  ezTime m_LastInteraction;

  ezVec2 m_MousePos;

  ezEngineGizmoHandle m_Corners;
  ezEngineGizmoHandle m_Edges[3];
  ezEngineGizmoHandle m_Faces[3];

  enum class ManipulateMode
  {
    None,
    Uniform,
    AxisX,
    AxisY,
    AxisZ,
    PlaneXY,
    PlaneXZ,
    PlaneYZ,
  };

  ManipulateMode m_ManipulateMode;

  ezVec3 m_vSize;
};
